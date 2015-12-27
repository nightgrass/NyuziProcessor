#
# Copyright 2011-2015 Jeff Bush
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#     http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
#

#
# This checks that, if an interrupt comes in during a gather load, it resumes
# from the correct lane. In this test, v0 is both the vector of pointers and the
# destination. If it restarted from 0 after returning from the interrupt, it would
# use the data value as a pointer. The data value is an odd number, which will
# cause an alignment exception if accessed as such.
#

.set CR_FAULT_HANDLER, 1
.set CR_FLAGS, 4
.set CR_FAULT_REASON, 3

				.globl _start
_start:			getcr s0, 0					# Find my thread ID
				btrue s0, spinner_thread	# If it is 1, jump to spinner thread

				# Start spinner thread, which is described below
				load_32 s0, threadresume
				move s1, 3	# Two threads enabled
				store_32 s1, (s0)

				# Set up interrupts
				lea s10, interrupt_handler
			    setcr s10, CR_FAULT_HANDLER	# Set fault handler address
				move s10, 5
				setcr s10, CR_FLAGS   		# Enable interrupts

				move s0, 1024				# Number of loops
loop0:
				# Invalidate the second page so we will miss the cache part way through
				# fetching these
				lea s1, dataloc2
				dinvalidate s1
				load_v v0, ptrvec 	# Get vector of pointers
				load_gath v0, (v0) 	# Gather load from pointers
				sub_i s0, s0, 1		# Decrement counter and loop if not done
				btrue s0, loop0

				# Write the PASS message
				load_32 s1, serialwrite
				move s0, 'P'
				store_32 s0, (s1)
				move s0, 'A'
				store_32 s0, (s1)
				move s0, 'S'
				store_32 s0, (s1)
				move s0, 'S'
				store_32 s0, (s1)
				goto halt

interrupt_handler: getcr s12, CR_FAULT_REASON
				cmpeq_i s13, s12, 3
				bfalse s13, bad_int
				eret

				# Write the FAIL message
bad_int:		load_32 s1, serialwrite
				move s0, 'F'
				store_32 s0, (s1)
				move s0, 'A'
				store_32 s0, (s1)
				move s0, 'I'
				store_32 s0, (s1)
				move s0, 'L'
				store_32 s0, (s1)

				# Halt the test
halt:			load_32 s0, threadhalt
				move s1, 3			# Thread 0 & 1
				store_32 s1, (s0)	# Halt thread

# The purpose of the spinner thread is to issue instructions in between the gather
# instructions from thread 0 and reset the subcycle counter. Otherwise it is still
# the value of the last instruction and the test appears to pass.
spinner_thread:	goto spinner_thread

threadresume:	.long 0xffff0060
threadhalt:		.long 0xffff0064
serialwrite:	.long 0xffff0020
				.align 64

# There will be a cache miss between dataloc1 and dataloc2, which will allow the interrupt to
# be dispatched.
ptrvec:			.long dataloc1, dataloc2, dataloc2, dataloc2, dataloc2, dataloc2, dataloc2, dataloc2
				.long dataloc2, dataloc2, dataloc2, dataloc2, dataloc2, dataloc2, dataloc2, dataloc2
dataloc1:		.long 7	 ; Odd address, will generate fault if used as pointer
				.align 64
dataloc2:		.long 5	 ; Also odd, but this is on a different cache line
