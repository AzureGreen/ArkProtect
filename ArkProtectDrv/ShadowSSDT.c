#include "ShadowSSDT.h"


BOOLEAN
APGetKeServiceDescriptorTableShadow(OUT PUINT_PTR SSSDTAddress)
{
#ifdef _WIN64
	/*
		kd> rdmsr c0000082
		msr[c0000082] = fffff800`03e85bc0

	*/
	PUINT8	StartSearchAddress = (PUINT8)__readmsr(0xC0000082);   // fffff800`03e85bc0
	PUINT8	EndSearchAddress = StartSearchAddress + 0x500;
	PUINT8	i = NULL;
	UINT8   v1 = 0, v2 = 0, v3 = 0;
	INT32   iOffset = 0;    // 002320c7 偏移不会超过4字节
	UINT64  VariableAddress = 0;

	*SSSDTAddress = 0;
	for (i = StartSearchAddress; i<EndSearchAddress; i++)
	{
		/*
			kd> u fffff800`03e85bc0 l 50
			nt!KiSystemCall64:
			fffff800`03e85bc0 0f01f8          swapgs
			......
			nt!KiSystemServiceRepeat:
			fffff800`03e85cf2 4c8d15478c2300  lea     r10,[nt!KeServiceDescriptorTable (fffff800`040be940)]
			fffff800`03e85cf9 4c8d1d808c2300  lea     r11,[nt!KeServiceDescriptorTableShadow (fffff800`040be980)]
			fffff800`03e85d00 f7830001000080000000 test dword ptr [rbx+100h],80h

			KeServiceDescriptorTableShadow:
			kd> dq fffff800`040be980
			fffff800`040be980  fffff800`03e87800 00000000`00000000
			fffff800`040be990  00000000`00000191 fffff800`03e8848c
			fffff800`040be9a0  fffff960`00191f00 00000000`00000000
			fffff800`040be9b0  00000000`0000033b fffff960`00193c1c

		*/

		if (MmIsAddressValid(i) && MmIsAddressValid(i + 1) && MmIsAddressValid(i + 2))
		{
			v1 = *i;
			v2 = *(i + 1);
			v3 = *(i + 2);
			if (v1 == 0x4c && v2 == 0x8d && v3 == 0x1d)		// 硬编码  lea r11
			{
				RtlCopyMemory(&iOffset, i + 3, 4);
				// 拿到了ShadowServiceDescriptorTable地址，他是一个数组，第一个成员是SSDT，第二个是SSSDT
				*SSSDTAddress = iOffset + (UINT64)i + 7;
				*SSSDTAddress += sizeof(UINT_PTR) * 4;		// 过SSDT
				break;
			}
		}
	}

	if (*SSSDTAddress == 0)
	{
		return FALSE;
	}

#else



#endif

	DbgPrint("SSDTAddress is %p\r\n", *SSSDTAddress);

	return TRUE;
}
