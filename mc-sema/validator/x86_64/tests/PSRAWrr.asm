BITS 64
;TEST_FILE_META_BEGIN
;TEST_TYPE=TEST_F
;TEST_IGNOREFLAGS=
;TEST_FILE_META_END

;TEST_BEGIN_RECORDING
lea rcx, [rsp-0x10]
mov dword [rcx+0x00], 0x0
mov dword [rcx+0x04], 0x0
mov dword [rcx+0x08], 0x0
mov dword [rcx+0x0C], 0x8
;set up ecx to be 8
movdqu xmm1, [rcx]

mov dword [rcx+0x00], 0xF0F0F0F0
mov dword [rcx+0x04], 0xF0F0F0F0
mov dword [rcx+0x08], 0xF0F0F0F0
mov dword [rcx+0x0C], 0xF0F0F0F0
movdqu xmm0, [rcx]

psraw xmm0, xmm1
mov ecx, 0
;TEST_END_RECORDING

cvtsi2sd xmm0, ecx
cvtsi2sd xmm1, ecx 
