#include <format>
#include "debugger.h"

static std::string disassemble_cb(std::shared_ptr<dmg::bus::Bus> bus, uint8_t pc) {
	switch (bus->ReadMemory(pc)) {
		case 0x00: return "RLC B";
		case 0x01: return "RLC C";
		case 0x02: return "RLC D";
		case 0x03: return "RLC E";
		case 0x04: return "RLC H";
		case 0x05: return "RLC L";
		case 0x06: return "RLC (HL)";
		case 0x07: return "RLC A";
		case 0x08: return "RRC B";
		case 0x09: return "RRC C";
		case 0x0a: return "RRC D";
		case 0x0b: return "RRC E";
		case 0x0c: return "RRC H";
		case 0x0d: return "RRC L";
		case 0x0e: return "RRC (HL)";
		case 0x0f: return "RRC A";
		case 0x10: return "RL B";
		case 0x11: return "RL C";
		case 0x12: return "RL D";
		case 0x13: return "RL E";
		case 0x14: return "RL H";
		case 0x15: return "RL L";
		case 0x16: return "RL (HL)";
		case 0x17: return "RL A";
		case 0x18: return "RR B";
		case 0x19: return "RR C";
		case 0x1a: return "RR D";
		case 0x1b: return "RR E";
		case 0x1c: return "RR H";
		case 0x1d: return "RR L";
		case 0x1e: return "RR (HL)";
		case 0x1f: return "RR A";
		case 0x20: return "SLA B";
		case 0x21: return "SLA C";
		case 0x22: return "SLA D";
		case 0x23: return "SLA E";
		case 0x24: return "SLA H";
		case 0x25: return "SLA L";
		case 0x26: return "SLA (HL)";
		case 0x27: return "SLA A";
		case 0x28: return "SRA B";
		case 0x29: return "SRA C";
		case 0x2a: return "SRA D";
		case 0x2b: return "SRA E";
		case 0x2c: return "SRA H";
		case 0x2d: return "SRA L";
		case 0x2e: return "SRA (HL)";
		case 0x2f: return "SRA A";
		case 0x30: return "SWAP B";
		case 0x31: return "SWAP C";
		case 0x32: return "SWAP D";
		case 0x33: return "SWAP E";
		case 0x34: return "SWAP H";
		case 0x35: return "SWAP L";
		case 0x36: return "SWAP (HL)";
		case 0x37: return "SWAP A";
		case 0x38: return "SRL B";
		case 0x39: return "SRL C";
		case 0x3a: return "SRL D";
		case 0x3b: return "SRL E";
		case 0x3c: return "SRL H";
		case 0x3d: return "SRL L";
		case 0x3e: return "SRL (HL)";
		case 0x3f: return "SRL A";
		case 0x40: return "BIT 0, B";
		case 0x41: return "BIT 0, C";
		case 0x42: return "BIT 0, D";
		case 0x43: return "BIT 0, E";
		case 0x44: return "BIT 0, H";
		case 0x45: return "BIT 0, L";
		case 0x46: return "BIT 0, (HL)";
		case 0x47: return "BIT 1, A";
		case 0x48: return "BIT 1, B";
		case 0x49: return "BIT 1, C";
		case 0x4a: return "BIT 1, D";
		case 0x4b: return "BIT 1, E";
		case 0x4c: return "BIT 1, H";
		case 0x4d: return "BIT 1, L";
		case 0x4e: return "BIT 1, (HL)";
		case 0x4f: return "BIT 1, A";
		case 0x50: return "BIT 2, B";
		case 0x51: return "BIT 2, C";
		case 0x52: return "BIT 2, D";
		case 0x53: return "BIT 2, E";
		case 0x54: return "BIT 2, H";
		case 0x55: return "BIT 2, L";
		case 0x56: return "BIT 2, (HL)";
		case 0x57: return "BIT 3, A";
		case 0x58: return "BIT 3, B";
		case 0x59: return "BIT 3, C";
		case 0x5a: return "BIT 3, D";
		case 0x5b: return "BIT 3, E";
		case 0x5c: return "BIT 3, H";
		case 0x5d: return "BIT 3, L";
		case 0x5e: return "BIT 3, (HL)";
		case 0x5f: return "BIT 3, A";
		case 0x60: return "BIT 4, B";
		case 0x61: return "BIT 4, C";
		case 0x62: return "BIT 4, D";
		case 0x63: return "BIT 4, E";
		case 0x64: return "BIT 4, H";
		case 0x65: return "BIT 4, L";
		case 0x66: return "BIT 4, (HL)";
		case 0x67: return "BIT 5, A";
		case 0x68: return "BIT 5, B";
		case 0x69: return "BIT 5, C";
		case 0x6a: return "BIT 5, D";
		case 0x6b: return "BIT 5, E";
		case 0x6c: return "BIT 5, H";
		case 0x6d: return "BIT 5, L";
		case 0x6e: return "BIT 5, (HL)";
		case 0x6f: return "BIT 5, A";
		case 0x70: return "BIT 6, B";
		case 0x71: return "BIT 6, C";
		case 0x72: return "BIT 6, D";
		case 0x73: return "BIT 6, E";
		case 0x74: return "BIT 6, H";
		case 0x75: return "BIT 6, L";
		case 0x76: return "BIT 6, (HL)";
		case 0x77: return "BIT 7, A";
		case 0x78: return "BIT 7, B";
		case 0x79: return "BIT 7, C";
		case 0x7a: return "BIT 7, D";
		case 0x7b: return "BIT 7, E";
		case 0x7c: return "BIT 7, H";
		case 0x7d: return "BIT 7, L";
		case 0x7e: return "BIT 7, (HL)";
		case 0x7f: return "BIT 7, A";
		case 0x80: return "RES 0, B";
		case 0x81: return "RES 0, C";
		case 0x82: return "RES 0, D";
		case 0x83: return "RES 0, E";
		case 0x84: return "RES 0, H";
		case 0x85: return "RES 0, L";
		case 0x86: return "RES 0, (HL)";
		case 0x87: return "RES 1, A";
		case 0x88: return "RES 1, B";
		case 0x89: return "RES 1, C";
		case 0x8a: return "RES 1, D";
		case 0x8b: return "RES 1, E";
		case 0x8c: return "RES 1, H";
		case 0x8d: return "RES 1, L";
		case 0x8e: return "RES 1, (HL)";
		case 0x8f: return "RES 1, A";
		case 0x90: return "RES 2, B";
		case 0x91: return "RES 2, C";
		case 0x92: return "RES 2, D";
		case 0x93: return "RES 2, E";
		case 0x94: return "RES 2, H";
		case 0x95: return "RES 2, L";
		case 0x96: return "RES 2, (HL)";
		case 0x97: return "RES 3, A";
		case 0x98: return "RES 3, B";
		case 0x99: return "RES 3, C";
		case 0x9a: return "RES 3, D";
		case 0x9b: return "RES 3, E";
		case 0x9c: return "RES 3, H";
		case 0x9d: return "RES 3, L";
		case 0x9e: return "RES 3, (HL)";
		case 0x9f: return "RES 3, A";
		case 0xa0: return "RES 4, B";
		case 0xa1: return "RES 4, C";
		case 0xa2: return "RES 4, D";
		case 0xa3: return "RES 4, E";
		case 0xa4: return "RES 4, H";
		case 0xa5: return "RES 4, L";
		case 0xa6: return "RES 4, (HL)";
		case 0xa7: return "RES 5, A";
		case 0xa8: return "RES 5, B";
		case 0xa9: return "RES 5, C";
		case 0xaa: return "RES 5, D";
		case 0xab: return "RES 5, E";
		case 0xac: return "RES 5, H";
		case 0xad: return "RES 5, L";
		case 0xae: return "RES 5, (HL)";
		case 0xaf: return "RES 5, A";
		case 0xb0: return "RES 6, B";
		case 0xb1: return "RES 6, C";
		case 0xb2: return "RES 6, D";
		case 0xb3: return "RES 6, E";
		case 0xb4: return "RES 6, H";
		case 0xb5: return "RES 6, L";
		case 0xb6: return "RES 6, (HL)";
		case 0xb7: return "RES 7, A";
		case 0xb8: return "RES 7, B";
		case 0xb9: return "RES 7, C";
		case 0xba: return "RES 7, D";
		case 0xbb: return "RES 7, E";
		case 0xbc: return "RES 7, H";
		case 0xbd: return "RES 7, L";
		case 0xbe: return "RES 7, (HL)";
		case 0xbf: return "RES 7, A";
		case 0xc0: return "SET 0, B";
		case 0xc1: return "SET 0, C";
		case 0xc2: return "SET 0, D";
		case 0xc3: return "SET 0, E";
		case 0xc4: return "SET 0, H";
		case 0xc5: return "SET 0, L";
		case 0xc6: return "SET 0, (HL)";
		case 0xc7: return "SET 1, A";
		case 0xc8: return "SET 1, B";
		case 0xc9: return "SET 1, C";
		case 0xca: return "SET 1, D";
		case 0xcb: return "SET 1, E";
		case 0xcc: return "SET 1, H";
		case 0xcd: return "SET 1, L";
		case 0xce: return "SET 1, (HL)";
		case 0xcf: return "SET 1, A";
		case 0xd0: return "SET 2, B";
		case 0xd1: return "SET 2, C";
		case 0xd2: return "SET 2, D";
		case 0xd3: return "SET 2, E";
		case 0xd4: return "SET 2, H";
		case 0xd5: return "SET 2, L";
		case 0xd6: return "SET 2, (HL)";
		case 0xd7: return "SET 3, A";
		case 0xd8: return "SET 3, B";
		case 0xd9: return "SET 3, C";
		case 0xda: return "SET 3, D";
		case 0xdb: return "SET 3, E";
		case 0xdc: return "SET 3, H";
		case 0xdd: return "SET 3, L";
		case 0xde: return "SET 3, (HL)";
		case 0xdf: return "SET 3, A";
		case 0xe0: return "SET 4, B";
		case 0xe1: return "SET 4, C";
		case 0xe2: return "SET 4, D";
		case 0xe3: return "SET 4, E";
		case 0xe4: return "SET 4, H";
		case 0xe5: return "SET 4, L";
		case 0xe6: return "SET 4, (HL)";
		case 0xe7: return "SET 5, A";
		case 0xe8: return "SET 5, B";
		case 0xe9: return "SET 5, C";
		case 0xea: return "SET 5, D";
		case 0xeb: return "SET 5, E";
		case 0xec: return "SET 5, H";
		case 0xed: return "SET 5, L";
		case 0xee: return "SET 5, (HL)";
		case 0xef: return "SET 5, A";
		case 0xf0: return "SET 6, B";
		case 0xf1: return "SET 6, C";
		case 0xf2: return "SET 6, D";
		case 0xf3: return "SET 6, E";
		case 0xf4: return "SET 6, H";
		case 0xf5: return "SET 6, L";
		case 0xf6: return "SET 6, (HL)";
		case 0xf7: return "SET 7, A";
		case 0xf8: return "SET 7, B";
		case 0xf9: return "SET 7, C";
		case 0xfa: return "SET 7, D";
		case 0xfb: return "SET 7, E";
		case 0xfc: return "SET 7, H";
		case 0xfd: return "SET 7, L";
		case 0xfe: return "SET 7, (HL)";
		case 0xff: return "SET 7, A";
		default: return "-";
	}
}

const char* op_table[] = {
	"NOP", 		"", 			"LD (BC), A", 	"INC BC", 		"INC B", 		"DEC B", 		"", 			"RLCA", 		"", 			"ADD HL, BC", 	"LD A, (BC)", 	"DEC BC",
	"INC C", 	"DEC C", 		"", 			"RRCA", 		"", 			"", 			"LD (DE), A", 	"INC DE", 		"INC D", 		"DEC D", 		"", 			"RLA",
	"",			"ADD HL, DE",	"LD A, (DE)", 	"DEC DE",		"INC E",		"DEC E",		"",				"RRA",			"",				"",				"LD (HL+), A",	"INC HL",
	"INC H",	"DEC H",	 	"",				"DAA",			"",				"ADD HL, HL",	"LD A, (HL+)",	"DEC HL",		"INC L",		"DEC L",		"",				"CPL",
	"", "", 	"LD (HL-), A", 	"INC SP",		"INC (HL)",		"DEC (HL)", 	"", 			"SCF", 			"", 			"ADD HL, SP", 	"LD A, (HL-)", 	"DEC SP", 		"INC A",
	"DEC A", 	"", 			"CCF", 			"LD B, B", 		"LD B, C", 		"LD B, D", 		"LD B, E", 		"LD B, H", 		"LD B, L", 		"LD B, (HL)", 	"LD B, A", 		"LD C, B",
	"LD C, C", 	"LD C, D", 		"LD C, E", 		"LD C, H", 		"LD C, L", 		"LD C, (HL)", 	"LD C, A", 		"LD D, B", 		"LD D, C", 		"LD D, D", 		"LD D, E", 		"LD D, H", 	
	"LD D, L", 	"LD D, (HL)", 	"LD D, A", 		"LD E, B", 		"LD E, C", 		"LD E, D", 		"LD E, E", 		"LD E, H", 		"LD E, L", 		"LD E, (HL)", 	"LD E, A", 		"LD H, B",
	"LD H, C", 	"LD H, D", 		"LD H, E", 		"LD H, H", 		"LD H, L", 		"LD H, (HL)", 	"LD H, A", 		"LD L, B", 		"LD L, C", 		"LD L, D", 		"LD L, E", 		"LD L, H", 
	"LD L, L", 	"LD L, (HL)", 	"LD L, A", 		"LD (HL), B", 	"LD (HL), C", 	"LD (HL), D", 	"LD (HL), E", 	"LD (HL), H", 	"LD (HL), L", 	"HALT", 		"LD A, A", 		"LD A, B", 
	"LD A, C", 	"LD A, D", 		"LD A, E", 		"LD A, H", 		"LD A, L", 		"LD A, (HL)", 	"LD A, A", 		"ADD A, B", 	"ADD A, C", 	"ADD A, D", 	"ADD A, E", 	"ADD A, H",
	"ADD A, L", "ADD A, (HL)",  "ADD A, A", 	"ADC A, B", 	"ADC A, C", 	"ADC A, D", 	"ADC A, E", 	"ADC A, H", 	"ADC A, L", 	"ADC A, (HL)", 	"ADC A, A", 	"SUB A, B",
	"SUB A, C", "SUB A, D", 	"SUB A, E", 	"SUB A, H", 	"SUB A, L", 	"SUB A, (HL)", 	"SUB A, A", 	"SBC A, B", 	"SBC A, C", 	"SBC A, D", 	"SBC A, E", 	"SBC A, H",
	"SBC A, L", "SBC A, (HL)", 	"SBC A, A", 	"AND A, B", 	"AND A, C", 	"AND A, D", 	"AND A, E", 	"AND A, H", 	"AND A, L", 	"AND A, (HL)", 	"AND A, A", 	"XOR A, B",
	"XOR A, C", "XOR A, D", 	"XOR A, E", 	"XOR A, H", 	"XOR A, L", 	"XOR A, (HL)", 	"XOR A, A", 	"OR A, B", 		"OR A, C", 		"OR A, D", 		"OR A, E", 		"OR A, H",
	"OR A, L", 	"OR A, (HL)", 	"OR A, A", 		"CP A, B", 		"CP A, C", 		"CP A, D", 		"CP A, E", 		"CP A, H", 		"CP A, L", 		"CP A, (HL)", 	"CP A, A", 		"RET NZ",
	"POP BC", 	"", 			"", 			"", 			"PUSH BC", 		"", 			"RST $00", 		"RET Z", 		"RET", 			"", 			"", 			"",
	"",			"", 			"RST $08", 		"RET NC", 		"POP DE", 		"", 			"-", 			"", 			"PUSH DE", 		"", 			"RST $10", 		"RET C",
	"RETI", 	"", 			"-", 			"", 			"-", 			"", 			"RST $18", 		"", 			"POP HL", 		"LDH (C), A", 	"-", 			"-",
	"PUSH HL", 	"", 			"RST $20", 		"", 			"JP HL", 		"", 			"-", 			"-", 			"-", 			"", 			"RST $28", 		"",
	"POP AF", 	"LDH A, (C)", 	"DI", 			"-", 			"PUSH AF", 		"", 			"RST $30", 		"", 			"LD SP, HL", 	"", 			"EI", 			"-",
	"-", 		"", 			"RST $38"
};

std::string dmg::debugger::DisassembleInstruction(std::shared_ptr<dmg::bus::Bus> bus, uint16_t pc) {
	uint16_t n16 = bus->ReadMemory16(pc + 1);
	uint8_t n8 = bus->ReadMemory(pc + 1);
	int8_t e8 = static_cast<int8_t>(n8);

	uint8_t opcode = bus->ReadMemory(pc);
	switch (opcode) {
		case 0x01: return std::format("LD BC, ${:04x}", n16);
		case 0x06: return std::format("LD B, ${:02x}", n8);
		case 0x08: return std::format("LD (${:04x}), SP", n16);
		case 0x0e: return std::format("LD C, ${:02x}", n8);
		case 0x10: return std::format("STOP ${:02x}", n8);
		case 0x11: return std::format("LD DE, ${:04x}", n16);
		case 0x16: return std::format("LD D, ${:02x}", n8);
		case 0x18: return std::format("JR {}{:02x}", e8 < -0xa || e8 > 9 ? "$" : "", e8);
		case 0x1e: return std::format("LD E, ${:02x}", n8);
		case 0x20: return std::format("JR NZ, {}{:02x}", e8 < -0xa || e8 > 9 ? "$" : "", e8);
		case 0x21: return std::format("LD HL, ${:04x}", n16);
		case 0x26: return std::format("LD H, ${:02x}", n8);
		case 0x28: return std::format("JR Z, {}{:02x}", e8 < -0xa || e8 > 9 ? "$" : "", e8);
		case 0x2e: return std::format("LD L, ${:02x}", n8);
		case 0x30: return std::format("JR NC, {}{:02x}", e8 < -0xa || e8 > 9 ? "$" : "", e8);
		case 0x31: return std::format("LD SP, ${:04x}", n16);
		case 0x36: return std::format("LD (HL), ${:02x}", n8);
		case 0x38: return std::format("JR C, {}{:02x}", e8 < -0xa || e8 > 9 ? "$" : "", e8);
		case 0x3e: return std::format("LD A, ${:02x}", n8);
		case 0xc2: return std::format("JP NZ, ${:04x}", n16);
		case 0xc3: return std::format("JP ${:04x}", n16);
		case 0xc4: return std::format("CALL NZ, ${:04x}", n16);
		case 0xc6: return std::format("ADD A, ${:02x}", n8);
		case 0xca: return std::format("JP Z, ${:04x}", n16);
		case 0xcb: return disassemble_cb(bus, pc + 1);
		case 0xcc: return std::format("CALL Z, ${:04x}", n16);
		case 0xcd: return std::format("CALL ${:04x}", n16);
		case 0xce: return std::format("ADC A, ${:02x}", n8);
		case 0xd2: return std::format("JP NC, ${:04x}", n16);
		case 0xd4: return std::format("CALL NC, ${:04x}", n16);
		case 0xd6: return std::format("SUB A, ${:02x}", n8);
		case 0xda: return std::format("JP C, ${:04x}", n16);
		case 0xdc: return std::format("CALL C, ${:04x}", n16);
		case 0xde: return std::format("SBC A, ${:02x}", n8);
		case 0xe0: return std::format("LDH (${:02x}), A", n8);
		case 0xe6: return std::format("AND A, ${:02x}", n8);
		case 0xe8: return std::format("ADD SP, ${:02x}", n8);
		case 0xea: return std::format("LD (${:04x}), A", n16);
		case 0xee: return std::format("XOR A, ${:02x}", n8);
		case 0xf0: return std::format("LDH A, (${:02x})", n8);
		case 0xf6: return std::format("OR A, ${:02x}", n8);
		case 0xf8: return std::format("LD HL, SP + {}{:02x}", e8 < -0xa || e8 > 9 ? "$" : "", e8);
		case 0xfa: return std::format("LD A, (${:04x})", n16);
		case 0xfe: return std::format("CP A, ${:02x}", n8);
		default: return op_table[opcode];
	}
}