BITS 32
;TEST_FILE_META_BEGIN
;TEST_TYPE=TEST_F
;TEST_IGNOREFLAGS=FLAG_SF|FLAG_PF
;TEST_FILE_META_END

;TEST_BEGIN_RECORDING
lea ecx, [esp-0x30]
and ecx, 0xFFFFFFF0

mov dword [ecx+0x00], 0xEEEEEEEE
mov dword [ecx+0x04], 0xFFFFFFFF
mov dword [ecx+0x08], 0x11111111
mov dword [ecx+0x0C], 0x22222222
movdqu xmm0, [ecx]

mov dword [ecx+0x00], 0xAAAAAAAA
mov dword [ecx+0x04], 0xBBBBBBBB
mov dword [ecx+0x08], 0xCCCCCCCC
mov dword [ecx+0x0C], 0xDDDDDDDD

shufps xmm0, [ecx], 0x4E
mov ecx, 0
;TEST_END_RECORDING

cvtsi2sd xmm0, ecx
