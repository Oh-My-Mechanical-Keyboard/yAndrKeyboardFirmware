	.cpu cortex-m4
	.arch armv7e-m
	.fpu softvfp
	.eabi_attribute 20, 1
	.eabi_attribute 21, 1
	.eabi_attribute 23, 3
	.eabi_attribute 24, 1
	.eabi_attribute 25, 1
	.eabi_attribute 26, 1
	.eabi_attribute 30, 4
	.eabi_attribute 34, 1
	.eabi_attribute 38, 1
	.eabi_attribute 18, 4
	.file	"bsp_btn_ble.c"
	.text
	.section	.text.advertising_buttons_configure,"ax",%progbits
	.align	1
	.syntax unified
	.thumb
	.thumb_func
	.type	advertising_buttons_configure, %function
advertising_buttons_configure:
	@ args = 0, pretend = 0, frame = 0
	@ frame_needed = 0, uses_anonymous_args = 0
	push	{r3, lr}
	movs	r2, #1
	movs	r1, #2
	movs	r0, #0
	bl	bsp_event_to_button_action_assign
	cbz	r0, .L2
	cmp	r0, #7
	bne	.L1
.L2:
	movs	r2, #7
	movs	r1, #2
	movs	r0, #1
	bl	bsp_event_to_button_action_assign
	cbz	r0, .L4
	cmp	r0, #7
	bne	.L1
.L4:
	movs	r1, #0
	movs	r2, #10
	mov	r0, r1
	bl	bsp_event_to_button_action_assign
	cbz	r0, .L1
	cmp	r0, #7
	it	eq
	moveq	r0, #0
.L1:
	pop	{r3, pc}
	.size	advertising_buttons_configure, .-advertising_buttons_configure
	.section	.text.ble_evt_handler,"ax",%progbits
	.align	1
	.syntax unified
	.thumb
	.thumb_func
	.type	ble_evt_handler, %function
ble_evt_handler:
	@ args = 0, pretend = 0, frame = 0
	@ frame_needed = 0, uses_anonymous_args = 0
	ldrh	r3, [r0]
	cmp	r3, #16
	push	{r4, lr}
	beq	.L16
	cmp	r3, #17
	beq	.L17
.L15:
	pop	{r4, pc}
.L16:
	ldr	r4, .L48
	ldr	r1, [r4]
	cbnz	r1, .L20
	movs	r2, #1
	mov	r0, r1
	bl	bsp_event_to_button_action_assign
	cbz	r0, .L21
	cmp	r0, #7
	bne	.L22
.L21:
	movs	r2, #7
	movs	r1, #2
	movs	r0, #1
	bl	bsp_event_to_button_action_assign
	cbz	r0, .L23
	cmp	r0, #7
	bne	.L22
.L23:
	movs	r2, #4
	movs	r1, #2
	movs	r0, #0
	bl	bsp_event_to_button_action_assign
	cbz	r0, .L20
	cmp	r0, #7
	beq	.L20
.L22:
	ldr	r3, .L48+4
	ldr	r3, [r3]
	cbz	r3, .L20
	blx	r3
.L20:
	ldr	r3, [r4]
	adds	r3, r3, #1
	str	r3, [r4]
	b	.L15
.L17:
	ldr	r2, .L48
	ldr	r3, [r2]
	subs	r3, r3, #1
	str	r3, [r2]
	cmp	r3, #0
	bne	.L15
	bl	advertising_buttons_configure
	cmp	r0, #0
	beq	.L15
	ldr	r3, .L48+4
	ldr	r3, [r3]
	cmp	r3, #0
	beq	.L15
	pop	{r4, lr}
	bx	r3	@ indirect register sibling call
.L49:
	.align	2
.L48:
	.word	m_num_connections
	.word	m_error_handler
	.size	ble_evt_handler, .-ble_evt_handler
	.section	.text.bsp_btn_ble_sleep_mode_prepare,"ax",%progbits
	.align	1
	.global	bsp_btn_ble_sleep_mode_prepare
	.syntax unified
	.thumb
	.thumb_func
	.type	bsp_btn_ble_sleep_mode_prepare, %function
bsp_btn_ble_sleep_mode_prepare:
	@ args = 0, pretend = 0, frame = 0
	@ frame_needed = 0, uses_anonymous_args = 0
	push	{r3, lr}
	movs	r0, #0
	bl	bsp_wakeup_button_enable
	cbz	r0, .L51
	cmp	r0, #6
	bne	.L50
.L51:
	movs	r0, #1
	bl	bsp_wakeup_button_enable
	cbz	r0, .L50
	cmp	r0, #6
	it	eq
	moveq	r0, #0
.L50:
	pop	{r3, pc}
	.size	bsp_btn_ble_sleep_mode_prepare, .-bsp_btn_ble_sleep_mode_prepare
	.section	.text.bsp_btn_ble_init,"ax",%progbits
	.align	1
	.global	bsp_btn_ble_init
	.syntax unified
	.thumb
	.thumb_func
	.type	bsp_btn_ble_init, %function
bsp_btn_ble_init:
	@ args = 0, pretend = 0, frame = 0
	@ frame_needed = 0, uses_anonymous_args = 0
	ldr	r3, .L69
	push	{r4, lr}
	str	r0, [r3]
	mov	r4, r1
	cbz	r1, .L61
	movs	r0, #1
	bl	bsp_button_is_pressed
	cbz	r0, .L62
	movs	r3, #2
.L68:
	strb	r3, [r4]
.L61:
	ldr	r3, .L69+4
	ldr	r3, [r3]
	cbnz	r3, .L64
	pop	{r4, lr}
	b	advertising_buttons_configure
.L62:
	bl	bsp_button_is_pressed
	cbz	r0, .L63
	movs	r3, #11
	b	.L68
.L63:
	strb	r0, [r4]
	b	.L61
.L64:
	movs	r0, #0
	pop	{r4, pc}
.L70:
	.align	2
.L69:
	.word	m_error_handler
	.word	m_num_connections
	.size	bsp_btn_ble_init, .-bsp_btn_ble_init
	.section	.sdh_ble_observers1,"a"
	.align	2
	.type	m_ble_observer, %object
	.size	m_ble_observer, 8
m_ble_observer:
	.word	ble_evt_handler
	.word	0
	.section	.bss.m_num_connections,"aw",%nobits
	.align	2
	.type	m_num_connections, %object
	.size	m_num_connections, 4
m_num_connections:
	.space	4
	.section	.bss.m_error_handler,"aw",%nobits
	.align	2
	.type	m_error_handler, %object
	.size	m_error_handler, 4
m_error_handler:
	.space	4
	.ident	"GCC: (based on arm-13.2.Rel1 source release) 13.2.1 20231009"
