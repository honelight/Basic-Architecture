	.ORIG x1234
	ADD R0, R0, R0
	BRzp label
	.FILL x0000	
label   .FILL x0000
	.END
