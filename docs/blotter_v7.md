Copied from: [Blotter File Format](https://gist.github.com/JimmyCushnie/bebea37a21acbb6e669589967f9512a2)

# Blotter File Format - v7

The Blotter File Format, successor to the format known as ".tung files", is a system for storing the components and wires of a Logic World world. It is used for both full world saves as well as subassemblies, but there are slight differences between the two.

## File types

* **World files:**
  * Represent a full, playable world space
  * May have zero or more root components at different positions/rotations in the world
  * Use `.logicworld` file extension
  * All circuit states are stored contiguously, from circuit state 0 through the highest state.
* **Subassembly files:**
  * Represent a substructure within a playable world space; can be loaded and added to a world at arbitrary position/rotation
  * Must have at least one root component
  * Use `.lwsubassembly` file extension
  * Has a list of circuit state indexes that are On. All indexes not listed here are inferred as being Off.

## File structure

* [Header](#header) (16 bytes)
* **Save info**
  * [Save Format Version](#save-format-version) (1 byte)
  * [Game version](#game-version) (16 bytes)
  * [Save type](#save-type) (1 byte)
  * [Number of components and wires](#component-and-wire-counts) (8 bytes)
  * [Mod versions](#mod-versions) (4+ bytes, dependent on the number of mods in the save)
  * [Component IDs Map](#component-ids-map) (4+ bytes, dependent on the number of loaded component IDs)
* **Object data**
  * [Components data](#components-data) (0+ bytes, dependent on the number of components)
  * [Wires data](#wires-data) (0+ bytes, dependent on the number of wires)
  * [Circuit states](#circuit-states) (4+ bytes, variable, has a different format for worlds and subassemblies)
* [Footer](#footer) (16 bytes)

The "Save Info" data is at the beginning of the file so that it is easy to extract as metadata. This is done by the game itself in a few places, as well as logicworld.net when uploading a save there.

## Header and footer

#### Header

The first 16 bytes of every Blotter file are `4C-6F-67-69-63-20-57-6F-72-6C-64-20-73-61-76-65`, which is UTF-8 for “Logic World save”. 

#### Footer

The final 16 bytes of every Blotter file are `72-65-64-73-74-6F-6E-65-20-73-75-78-20-6C-6F-6C `, which is UTF-8 for “redstone sux lol”.

The header and footer exist to protect against save corruption. If the file does not have them, we know that something is wrong with the file.

## Save Format Version

One byte specifies the save format version. Currently this is `07`.

## Game version

16 bytes store the game [version](#version) that this save was created in.

## Save type

There are two types of file that use the Blotter format, worlds and subassemblies. One byte specifies which type of file this is.

| Byte        | Meaning                 |
| ----------- | ----------------------- |
| `00`        | Unknown/error           |
| `01`        | World                   |
| `02`        | Subassembly             |
| `03` - `FF` | Reserved for future use |

## Component and wire counts

Here there are two [ints](#int). The first specifies the number of components in the save, the second specifies the number of wires.

## Mod versions

Mods can opt-in to storing their versions in save files created while they are installed. This is so that if a later mod version has changes, older save files can be converted.

* An [int](#int) for the number of mod versions stored in this file
* For each mod version:
  * A [string](#string) for the text ID of the mod
  * A [version](#version) for the version of the mod that this save was created with

## Component IDs Map

When humans add component types to Logic World, they use text IDs like `MHG.CircuitBoard`. To make save files and network packets small in size, the game internally uses numeric IDs to reference component types. The relationship between text IDs and numeric IDs is not deterministic or consistent between different save files, so the map from text IDs to numeric IDs must be stored with the save.

First we write an [int](#int) for the number of component IDs in this file.

One by one the mappings are written to the file:

* A 2-byte unsigned integer for the numeric ID of the component
* A [string](#string) for the text ID of the component

## Components data

One by one, every component is listed in the file. The first component must be a root component -- i.e. have an address of `C-0` for its parent -- and every subsequent component must have a parent that was listed before it.

It is explicitly okay to have a world file with no components in it (i.e. an empty world). However, a subassembly file must have at least one component.

Each component is written like so:

* The [component address](#component-address) of this component
* The [component address](#component-address) of the component's parent
* 2 bytes for the numeric ID of the component's type (see [Component IDs Map](#component-ids-map))
* Three [ints](#int) for the local fixed position of the component; `x y z`
  * One unit in the fixed position corresponds to a distance 0.001 game units, or 1mm
* Four [floats](#float) for the local rotation of the component as a quaternion; `x y z w`
* Information about the component's inputs:
  * An [int](#int) for the number of inputs the component has
  * For each input:
    * An [int](#int) for the input's circuit state ID
* Information about the component's outputs:

  * An [int](#int) for the number of outputs the component has
  * For each output:
    * An [int](#int) for the output's circuit state ID
* The component's custom data:
  * An [int](#int) for the number of bytes in the custom data
    * If the component has no custom data, `-1` may be written instead of `0`
  * All the bytes of the custom data, in order

## Wires data

One by one, every wire is listed in the file.

* The [peg address](#peg-address) of the wire's first point
* The [peg address](#peg-address) of the wire's second point
* An [int](#int) for the wire's circuit state ID
* A [float](#float) for the wire's rotation, relative to the default rotation it would have

We do not need to save wire addresses; nothing in the save file needs to reference them, and there is no need for them to be consistent between save loads. Instead, wire addresses are dynamically assigned when the save is loaded.

## Circuit states

We've saved the circuit state IDs of all the pegs and wires in the save. Now we need to save the values of those state IDs, so states can be looked up. 

A state is either on or off. At runtime, the game stores all circuit states from zero through the highest state in a contiguous block of memory.

### World files

Worlds are large, and generally use most circuit states, starting from state 0. All circuit states are listed.

* First an [int](#int) for the number of bytes in this sequence
* Then, that number of bytes. Each bit represents the value of the next circuit state; each byte contains 8 states.
  * If, when saving the game, the number of circuit states is not divisible by 8, the last byte will be padded.

### Subassembly files

Subassemblies represent a subset of a world. Most of the circuit states in a world will be irrelevant to a particular subassembly, so it would be silly and wasteful to save all of them with every subassembly.

Instead, we save a list of circuit state IDs which are both used in this subassembly and have a state of on. All unlisted state IDs are assumed to be in the off state.

* First an [int](#int) for the number of ints in this sequence
* Then, that number of ints. Each int signifies a state ID with a value of on.



## Sub-standards

**Int** (4 bytes)<a id="int"></a>  
32 bit signed integers, little endian.

**Float** (4 bytes)<a id="float"></a>  
32 bit floating point values, little endian.

**Boolean** (1 byte)<a id="boolean"></a>  
An entire byte that is either `00` for false or `01` for true.

**String** (4+ bytes)<a id="string"></a>  
An [int](#int) for the number of bytes in the string, followed by that number of bytes, representing that string in UTF-8.

**Version** (16 bytes)<a id="version"></a>  
Versions contain four numbers (i.e. 1.0.3.1069). These four numbers are written to the file in order, each as a 4-byte [int](#int). Thus, 4x4 bytes = 16 bytes for this.

**Component Address** (4 bytes)<a id="component-address"></a>  
Component Addresses are written as 32 bit unsigned integers, little endian.

**Peg Address** (9 bytes)<a id="peg-address"></a>

* One byte to indicate the peg type:

  | Byte        | Meaning                 |
  | ----------- | ----------------------- |
  | `00`        | Undefined               |
  | `01`        | Input peg               |
  | `02`        | Output peg              |
  | `03` - `FF` | Reserved for future use |

  Any value other than `01` or `02` is invalid.

* A [component address](#component-address) for the component of the peg

* An [int](#int) for the zero-based index of the peg (i.e. is it the 0th input, the 1st input, ect )

## A note on seemingly-bizarre design choices

This file structure is not the most efficient, most beautiful, or most logical way to lay out the data. The primary purpose of the design of the Blotter format is that it is easy to integrate them with the game code. Therefore, the format uses the exact same data structures used by the game. This makes it fast and simple to convert the game state to a save file and vice versa, but since these data structures are optimized for runtime, the files do look a little weird.

* **"Why are circuit states stored so weirdly? Why not directly save the state of an input/output, instead of a reference to a state index?"**
  * This is how the game stores circuit states at runtime, for performance and for ease of synchronization between the client and server. Storing it this way in the file makes for easy interoperability with the runtime game code, and allows us to use the same component/wire data structures in the saves and in the game.
* **"Must all the circuit states be stored? Surely you only need the states of the inputs or outputs, and everything else (including the wires) can be inferred from those."**
  * This is true for the server, but the client has no ability to run a simulation and infer other states. However, the client needs to know all the states so that it can render a subassembly correctly before it is placed and added to the server's world/simulation. Storing all the states is also just simpler, and requires fewer steps when loading a save.
* **"Why in god's name are all the integers signed when come on they really should be unsigned?"**
  * Logic World is written in C#. The .NET runtime has a few weird quirks, and one of those quirks is that most integers are signed, even things that can never be below zero like array lengths and indexes. Since this file format is designed for maximum interoperability with the code, this quirk is copied, and we use signed ints a lot.
