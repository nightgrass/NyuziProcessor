//
// Copyright 2015 Jeff Bush
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
//

// TLB flags
.set TLB_PRESENT, 1
.set TLB_WRITABLE, (1 << 1)
.set TLB_EXECUTABLE, (1 << 2)
.set TLB_SUPERVISOR, (1 << 3)
.set TLB_GLOBAL, (1 << 4)

// Control register indices
.set CR_CURRENT_THREAD, 0
.set CR_TRAP_HANDLER, 1
.set CR_TRAP_PC, 2
.set CR_TRAP_CAUSE, 3
.set CR_FLAGS, 4
.set CR_TRAP_ADDRESS, 5
.set CR_CYCLE_COUNT, 6
.set CR_TLB_MISS_HANDLER, 7
.set CR_SAVED_FLAGS, 8
.set CR_CURRENT_ASID, 9
.set CR_SCRATCHPAD0, 11
.set CR_SCRATCHPAD1, 12
.set CR_SUBCYCLE, 13
.set CR_INTERRUPT_ENABLE, 14
.set CR_INTERRUPT_ACK, 15
.set CR_INTERRUPT_PENDING, 16
.set CR_INTERRUPT_TRIGGER, 17
.set CR_JTAG_DATA, 18
.set CR_SYSCALL_INDEX, 19

// Trap types
.set TT_RESET, 0
.set TT_ILLEGAL_INSTRUCTION, 1
.set TT_PRIVILEGED_OP, 2
.set TT_INTERRUPT, 3
.set TT_SYSCALL, 4
.set TT_UNALIGNED_ACCESS, 5
.set TT_PAGE_FAULT, 6
.set TT_TLB_MISS, 7
.set TT_ILLEGAL_STORE, 8
.set TT_SUPERVISOR_ACCESS, 9
.set TT_NOT_EXECUTABLE, 10
.set TT_BREAKPOINT, 11

.set TRAP_CAUSE_STORE, 0x10
.set TRAP_CAUSE_DCACHE, 0x20

// Flag register bits
.set FLAG_INTERRUPT_EN, (1 << 0)
.set FLAG_MMU_EN, (1 << 1)
.set FLAG_SUPERVISOR_EN, (1 << 2)

// Device registers
.set REG_HOST_INTERRUPT, 0xffff0018
.set REG_SERIAL_WRITE, 0xffff0048
.set REG_THREAD_RESUME, 0xffff0100
.set REG_THREAD_HALT, 0xffff0104
.set REG_TIMER_COUNT, 0xffff0240

.macro start_all_threads
                li s0, REG_THREAD_RESUME
                li s1, 0xffffffff
                store_32 s1, (s0)
.endm

.macro halt_current_thread
                getcr s0, CR_CURRENT_THREAD
                move s1, 1
                shl s1, s1, s0
                li s0, REG_THREAD_HALT
                store_32 s1, (s0)
1:              b 1b
.endm

.macro halt_all_threads
                li s0, REG_THREAD_HALT
                li s1, 0xffffffff
                store_32 s1, (s0)
1:              b 1b
.endm

// Print a null terminated string pointer to by s0 to the serial
// port. Clobbers s0-s3.
                .align 4, 0xff
.macro print_string
                li s1, 0xffff0040       // Load address of serial registers
1:              load_u8 s2, (s0)        // Read a character
                bz s2, 3f               // If delimeter, exit
2:              load_32 s3, (s1)        // Read STATUS
                and s3, s3, 1           // Check write available bit
                bz s3, 2b               // If this is clear, busy wait
                store_32 s2, 8(s1)      // Write space available, send char
                add_i s0, s0, 1         // Increment pointer
                b 1b                    // Loop for next char
3:
.endm

// If register is not equal to testval, print failure message.
// Otherwise continue. Clobbers s25
.macro assert_reg reg, testval
                li s25, \testval
                cmpeq_i s25, s25, \reg
                bnz s25, 1f
                call fail_test
1:
.endm

.macro flush_pipeline
                b 1f
1:
.endm

.macro should_not_get_here
                call fail_test
.endm

// Print failure message and halt. The \x04 (^D) will cause the serial console
// to exit, triggering the end of the test on the FPGA target.
                .align 4, 0xff
fail_test:      lea s0, failstr
                print_string
                halt_all_threads
failstr:        .ascii "FAIL"
                .byte 4, 0

// Print success message and halt
                .align 4, 0xff
pass_test:      lea s0, passstr
                print_string
                halt_all_threads
passstr:        .ascii "PASS"
                .byte 4, 0

                .align 4, 0xff


