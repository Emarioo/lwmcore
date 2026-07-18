

We want to explore different encodings. In the assembler and emulator there are many places we encode and decode instructions.
We would like to specify the encoding in one place and have it generate encode for assembler and very efficient decode for emulator.
This one place would describe all instructions as well and generate the ISA.

Things to optimize for:
- Easy to implement decoding (easy in C sure but also logic gates in hardware and logic world)
- Fast decoding
- Efficient use of bytes. Variable-length instructions makes this easier but in turn complicates decoding.

# Easy decoding
Easy to implement encoding would be one byte for each opcode. Maybe extension opcode if we end up with more than 256.
Each opcode describes a basic form like 1/2/3 register operands. Memory form where we have 1 register operand and 1 memory operand. The 
memory operand has one byte for type which describes size of immedate (8,16,32,64 bits) and whether we have base and index register.

In C this is easy to decode because one big switch on the opcode and then if statmeents on memory operand type if we have it.

In hardware we would be a little smarter

