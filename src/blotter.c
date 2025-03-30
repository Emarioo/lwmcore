#include "lwmcore/blotter.h"

//###########################
//     PUBLIC FUNCTIONS
//###########################

BlotterData* decompose_blotter_file(string path) {
    FILE* file = fopen(path.ptr, "rb");
    if(!file) {
        error("Could not read '%s'\n", path.ptr);
        return nullptr;
    }
    fseek(file, 0, SEEK_END);
    int filesize = ftell(file);
    fseek(file, 0, SEEK_SET);
    char* raw = (char*)malloc(filesize);
    // TODO: Memory leak raw data, maybe BlotterData owns the memory and it should be freed when destroyed?
    Assert(raw);
    fread(raw, 1, filesize, file);
    fclose(file);
    string data = {filesize, raw};
    return decompose_blotter_data(data, path.ptr);
}
BlotterData* decompose_blotter_data(string data, const char* source_path) {
    if(data.len < 16) {
        error("File '%s' is corrupt! (to few bytes, %d)\n", source_path, data.len);
        return nullptr;
    }

    // TODO: Handle sudden EOF everywhere.

    //######################
    //    PARSE HEADER
    //######################

    BlotterHeader* header = (BlotterHeader*)(data.ptr + 0);
    BlotterFooter* footer = (BlotterFooter*)(data.ptr + data.len - 16);

    if(memcmp(header->magic, "Logic World save", 16)) {
        error("File '%s' is corrupt! (bad header)\n", source_path);
        return nullptr;
    }
    if(memcmp((char*)footer, "redstone sux lol", 16)) {
        error("File '%s' is corrupt! (bad footer)\n", source_path);
        return nullptr;
    }

    if(header->save_format_version != 7) {
        error("Can't handle save version %d. Only 7.\n", (int)header->save_format_version);
        return nullptr;
    }
    
    // nocheckin LOGGING
    printf("Game version: %d.%d.%d.%d\n", header->game_version[0], header->game_version[1], header->game_version[2], header->game_version[3]);

    const char* save_type = "Unknown";
    switch(header->save_type) {
        case LW_SAVE_TYPE_WORLD: save_type = "World"; break;
        case LW_SAVE_TYPE_SUBASSEMBLY: save_type = "Subassembly"; break;
        default:
            error("Expected save type 1 or 2 but got %d\n", (int)header->save_type);
            return nullptr;
    }
    printf("Save type: %s\n", save_type);

    // nocheckin LOGGING
    printf("Num components: %d\n", header->num_components);
    printf("Num wires: %d\n", header->num_wires);

    //####################################
    //    PARSE VARIABLE LENGTH DATA
    //####################################
    int head = sizeof(BlotterHeader);

    int num_mods = *(i32*)(data.ptr+head);
    head += 4;

    for(int mi=0;mi<num_mods;mi++) {
        string name = decompose_blotter_string(&data, &head);
        int version[4];
        memcpy(version, data.ptr + head, 16);
        head += 16; // skip version

        // nocheckin
        printf("Mod %s, %d.%d.%d.%d\n", name.ptr, version[0], version[1], version[2], version[3]);
    }

    int num_component_ids = *(i32*)(data.ptr+head);
    head+=4;

    // TODO: We allocate a lot of memory here. Use Hash map instead, i have to implement one in C first ):
    int component_map_size = 0x10000 * sizeof(string);
    string* component_map = malloc(component_map_size);
    memset(component_map, 0, component_map_size);

    // nocheckin logging
    printf("Components ids (%d):\n", num_component_ids);
    for(int ci=0;ci<num_component_ids;ci++) {
        i16 component_id = *(i16*)(data.ptr+head);
        head += 2;
        
        string name = decompose_blotter_string(&data, &head);

        // nocheckin LOGGING
        printf("  %d %s\n", (int)component_id, name.ptr);

        component_map[component_id] = name;
    }

    printf("Components (%d):\n", header->num_components);
    for(int ci=0;ci<header->num_components;ci++) {
        
        BlotterComponentHeader* component = (BlotterComponentHeader*)(data.ptr+head);
        head += sizeof(BlotterComponentHeader);

        int num_inputs = *(i32*)(data.ptr+head); 
        head += 4;
        int* inputs = (int*)(data.ptr+head); 
        head += num_inputs * sizeof(int);

        int num_outputs = *(i32*)(data.ptr+head); 
        head += 4;
        int* outputs = (int*)(data.ptr+head); 
        head += num_outputs * sizeof(int);

        int custom_size = *(i32*)(data.ptr+head); 
        head += 4;
        if(custom_size == -1)
            custom_size = 0;
        // TODO: Parse custom data?
        char* custom_data = (char*)(data.ptr+head);
        head += custom_size;

        string comp_name = component_map[component->comp_id];

        // nocheckin LOGGING
        float pos[3] = {component->position[0]*0.001f, component->position[1]*0.001f, component->position[2]*0.001f};
        printf(" 0x%x: %s %.2f %.2f %.2f\n", ((int)((char*)component - data.ptr)), comp_name.ptr, pos[0], pos[1], pos[2]);
        printf("    addr/parent: %d, %d\n", component->addr, component->parent_addr);
        if(custom_size) {
            printf("    custom data [0x%x - 0x%x] (%d)\n", (int)(custom_data - data.ptr), (int)(custom_data - data.ptr) + custom_size, custom_size);
            if(equal(comp_name, "MHG.CircuitBoard")) {
                BlotterCustomData_CircuitBoard* custom = (BlotterCustomData_CircuitBoard*)custom_data;
                printf("    RGB: %d, %d, %d\n", custom->red, custom->blue, custom->green);
                printf("    width/height: %d, %d\n", custom->width, custom->height);
            } else if(equal(comp_name, "MHG.PanelLabel")) {
                BlotterCustomData_PanelLabel* custom = (BlotterCustomData_PanelLabel*)custom_data;
                printf("    data0: %d\n", custom->_0);
                printf("    RGB: %d, %d, %d\n", custom->red, custom->blue, custom->green);
                printf("    data1: %x %x %x %x %x\n", custom->_1[0], custom->_1[1], custom->_1[2], custom->_1[3], custom->_1[4]);
                int temp_head = sizeof(BlotterCustomData_PanelLabel);
                string temp_data = { custom_size, custom_data };
                string text = decompose_blotter_string(&temp_data, &temp_head);
                int* nums = (int*)(custom_data + temp_head);
                
                printf("    text: \"");
                for(int i=0;i<text.len;i++) {
                    char chr = text.ptr[i];
                    if(chr == '\n') {
                        printf("\\n");
                    } else if(chr < 32) {
                        printf("\\x%X%X", chr&0xF0, chr&0xF);
                    } else {
                        printf("%c", chr);
                    }
                } printf("\"\n");

                printf("    data2: %d, %d, %d\n", nums[0], nums[1], nums[2]);
            } else  if(equal(comp_name, "MHG.Button")) {
                BlotterCustomData_Button* custom = (BlotterCustomData_Button*)custom_data;
                printf("    RGB: %d %d %d\n", custom->red, custom->green, custom->blue);
                printf("    active: %d\n", custom->active);
            } else  if(equal(comp_name, "MHG.Switch")) {
                BlotterCustomData_Switch* custom = (BlotterCustomData_Switch*)custom_data;
                printf("    RGB: %d %d %d\n", custom->red, custom->green, custom->blue);
                printf("    active: %d\n", custom->active);
            // } else  if(equal(comp_name, "MHG.Delayer")) {
            //     BlotterCustomData_Delayer* custom = (BlotterCustomData_Delayer*)custom_data;
            //     printf("    data0: %x %x %x %x\n", custom->_0[0], custom->_0[1], custom->_0[2], custom->_0[3]);
            //     printf("    data1: %d\n", custom->_1);
            } else {
                printf("    UNKNOWN\n");
                // printf("    custom data [0x%x - 0x%x[\n", (int)(custom_data - data.ptr), (int)(custom_data - data.ptr) + custom_size);
                // printf("    off: 0x%x\n", (int)(custom_data - data.ptr));
                // for(int i=0;i<custom_size;i++) {
                //     printf("    0x%x %d\n", *(int*)(custom_data+i), *(int*)(custom_data+i));
                //     i+=3;
                // }
            }
        }
    }

    printf("Wires (%d):\n", header->num_wires);
    for(int ci=0;ci<header->num_wires;ci++) {
        BlotterWire* wire = (BlotterWire*)(data.ptr+head);
        head += sizeof(BlotterWire);
    }

    if(header->save_type == LW_SAVE_TYPE_SUBASSEMBLY) {
        int num_states = *(i32*)(data.ptr+head);
        head += 4;
        int* circuit_ids = (i32*)(data.ptr+head);
        head += num_states * sizeof(int);

        printf("Circuit state ids (%d):\n", num_states);
    } else if(header->save_type == LW_SAVE_TYPE_WORLD) {
        int num_bytes = *(i32*)(data.ptr+head);
        head += 4;
        i8* state_data = (i8*)(data.ptr+head);
        head += num_bytes;

        printf("Circuit states (%d):\n", num_bytes * 8);
    } else Assert(false);

    if(head != data.len-16) {
        warning("%d bytes was not processed at the end '%s'.\n", data.len-16-head, source_path);
    }

    return nullptr;
}

void write_subassembly() {
    string data;
    int headmax = 0x10000;
    data.ptr = malloc(headmax);
    Assert(data.ptr);
    data.len = 0;

    int head = 0;

    // ################
    //    Write data
    // ################

    BlotterHeader* header = (BlotterHeader*)(data.ptr+head);
    head += sizeof(BlotterHeader);

    memcpy(header->magic, "Logic World save", 16);
    int default_version[4]={0,92,0,352};
    memcpy(header->game_version, default_version, 16);
    header->save_type = LW_SAVE_TYPE_SUBASSEMBLY;
    header->save_format_version = 7;
    header->num_components = 0;
    header->num_wires = 0;

    // Number of mods
    *(int*)(data.ptr+head) = 1;
    head += 4;

    // Write mod name
    *(int*)(data.ptr+head) = 3;
    head += 4;
    memcpy(data.ptr+head, "MHG", 3);
    head += 3;

    // Write mod version
    int default_mod_version[4]={0,91,4,-1};
    memcpy(data.ptr+head, default_mod_version, 16);
    head += 16;

    typedef struct {
        int id;
        const char* name;
    } Comp;
    Comp components[] = {
        0,  "MHG.Switch",
        2,  "MHG.Button",
        6,  "MHG.Inverter",
        8,  "MHG.AndGate",
        9,  "MHG.Delayer",
        10, "MHG.DLatch",
        19, "MHG.CircuitBoard",
        21, "MHG.Peg",
        25, "MHG.ChubbySocket",
        28, "MHG.PanelLabel",
    };
    int comp_len = sizeof(components)/sizeof(*components);
    
    *(i32*)(data.ptr+head) = comp_len;
    head+=4;

    for(int i=0;i<comp_len;i++) {
        *(u16*)(data.ptr+head) = components[i].id;
        head += 2;
        int namelen = strlen(components[i].name);
        *(int*)(data.ptr+head) = namelen;
        head += 4;
        memcpy(data.ptr+head, components[i].name, namelen);
        head += namelen;
    }

    // Write components
    {
        header->num_components++;
        BlotterComponentHeader* comp = (BlotterComponentHeader*)(data.ptr+head);
        head += sizeof(BlotterComponentHeader);

        comp->addr = 0x1000;
        comp->comp_id = 19;
        comp->parent_addr = 0;
        comp->position[0] = 0;
        comp->position[1] = 1;
        comp->position[2] = 2;
        comp->rotation[0] = 0;
        comp->rotation[1] = 0;
        comp->rotation[2] = 0;
        comp->rotation[3] = 1;
        
        // inputs
        *(int*)(data.ptr+head) = 0;
        head += 4;

        // outputs
        *(int*)(data.ptr+head) = 0;
        head += 4;

        // custom data
        *(int*)(data.ptr+head) = sizeof(BlotterCustomData_CircuitBoard);
        head += 4;

        BlotterCustomData_CircuitBoard* custom = (BlotterCustomData_CircuitBoard*)(data.ptr+head);
        head += sizeof(BlotterCustomData_CircuitBoard);
        custom->red = 240;
        custom->green = 50;
        custom->blue = 50;
        custom->width = 10;
        custom->height = 10;
    }

    // Wire data (nothing for now)
    
    // Circuit states (nothing for now)
    *(int*)(data.ptr+head) = 0;
    head += 4;

    BlotterFooter* footer = (BlotterFooter*)(data.ptr+head);
    head += sizeof(BlotterFooter);

    memcpy(footer->magic, "redstone sux lol", 16);

    // ###############
    //     Finish
    // ###############
    data.len = head;

    const char* path = "C:/Program Files (x86)/Steam/steamapps/common/Logic World/subassemblies/test/data.partialworld";
    // const char* path = "examples/test.partialworld";
    if(DISABLE_FILE_WRITES) {
        printf("fopen '%s' write %d bytes\n", path, data.len);
    } else {
        FILE* file = fopen(path, "wb");
        if(!file) {
            error("Could not open '%s'\n", path);
            return;
        }
        fwrite(data.ptr, 1, data.len, file); 
        fclose(file);
    }
}

bool write_metadata(string folder_path, const char* title, bool force) {
    // TODO: Memory leaks
    char* meta_path = malloc(500);
    char* placements_path = malloc(500);

    sprintf(meta_path, "%s/meta.jecs", folder_path.ptr);
    sprintf(placements_path, "%s/placements.jecs", folder_path.ptr);

    const char* KEYWORD = "# Generated by lwm";
    const int KEYWORD_LEN = strlen(KEYWORD);
    // Check if we're overwriting an existing user created subassembly
    if (!force) {
        FILE* meta = fopen(meta_path, "rb");
        if(meta) {
            char* data = malloc(20);
            fread(data, 1, KEYWORD_LEN, meta);
            fclose(meta);

            if(memcmp(data, KEYWORD, KEYWORD_LEN)) {
                error("'%s' is a user made subassembly (not made by lwm). Are you sure you want to overwrite it? (use --force if you are)\n", folder_path.ptr);
                return false;
            }
        }
    }

    bool res = create_folder(folder_path);
    if(!res)
        return res; // error already printed

    if(DISABLE_FILE_WRITES) {
        printf("fopen '%s'\n");
    } else {
        FILE* meta = fopen(meta_path, "wb");
        if(!meta) {
            error("Could not open '%s'.\n", meta_path);
            return false;
        }
        fprintf(meta, "%s\nTitle: %s\nColumn: null\nCategory: null\n", KEYWORD, title);
        fclose(meta);
    }
    if(DISABLE_FILE_WRITES) {
        printf("fopen '%s'\n");
    } else {
        FILE* placements = fopen(placements_path, "wb");
        if(!placements) {
            error("Could not open '%s'.\n", placements_path);
            return false;
        }
        fprintf(placements, "Placements:\n"
                            "    -\n"
                            "        CornerMidpointPositionBeforeFlipping:\n"
                            "            x: 0\n"
                            "            y: 0\n"
                            "            z: 0\n"
                            "        RotationBeforeFlipping:\n"
                            "            x: 0\n"
                            "            y: 0\n"
                            "            z: 0\n"
                            "            w: 1\n"
                            "        PlacementType: OnTerrain\n"
                            "        Flipped: false\n"
                            "        BoardRotationState: 0\n"
                            "        Type: Standard\n"
                            "data_format_version: 1\n");
        fclose(placements);
    }
    return true;
}

bool modify_rom_subassembly(string data, string* rom_data) {
    return true;
}

//###########################
//     PRIVATE FUNCTIONS
//###########################

string decompose_blotter_string(string* data, int* head) {
    // TODO: Handle EOF
    string name;
    name.len = *(i32*)(data->ptr+*head);
    *head += 4;

    name.ptr = malloc(name.len + 1);
    Assert(name.ptr);
    memcpy(name.ptr, data->ptr+*head, name.len);
    name.ptr[name.len] = '\0';
    *head += name.len;
    return name;
}