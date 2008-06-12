
# This is a general purpose bootloader routine. It sets up the stack register
# and frame, and jumps to the _main program location. It should be linked at
# the start of all programs.

	.file   "bootloader.s"

	.text
	.org	0x100		# The reset routine goes at 0x100

	.global _start
_start:
	l.addi	r1,r0,0x7f00	# Set SP to value 0x7f00
	l.addi	r2,r1,0x0	# FP and SP are the same
	l.mfspr	r3,r0,17	# Get SR value
	l.ori	r3,r3,0x10	# Set exception enable bit
	l.jal   _main		# Jump to main routine
	l.mtspr	r0,r3,17	# Enable exceptions (DELAY SLOT)

	.org	0xFFC
	l.nop			# Guarantee the exception vector space
				# does not have general purpose code

# C code starts at 0x1000
