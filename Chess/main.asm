.data
readFile: .asciiz "SampleBoards/case1.txt"
king_pos: .asciiz ""

.text
	li $a0, 0x9
	li $a1, 0x3
	li $a2, 0x7
	jal initBoard
	jal initPieces
	
	la $a0, readFile
	jal loadGame
	
	#jal initPieces
	
	li $a0, 0x44
	li $a1, 0x33
	jal mapChessMove
	
	move $s0, $v0
	
	li $a0, 0x42
	li $a1, 0x35
	jal mapChessMove
	
	li $a0, 1
	move $a1, $s0
	move $a2, $v0
	li $a3, 0x9
	la $t0, king_pos
	sw $t0, 0($sp)
	jal perform_move
	
	
	li $a0, 0x0105
	jal getChessPiece
	
	
	li $a0, 66
	li $a1, 54
	jal mapChessMove
	
	
	li $a0, 7
	li $a1, 3
	li $a2, 0x51
	li $a3, 2
	
	addi $sp, $sp, -4
	li $t0, 0x04
	sb $t0, 0($sp)
	
	#jal setSquare
	
	
.include "hw4_seokn.asm"
