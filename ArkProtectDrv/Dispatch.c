#include "Dispatch.h"



NTSTATUS
APIoControlPassThrough(IN PDEVICE_OBJECT DeviceObject, IN PIRP Irp)
{
	NTSTATUS			Status = STATUS_SUCCESS;
	PVOID				InputBuffer = NULL;
	PVOID               OutputBuffer = NULL;
	UINT32				InputLength = 0;
	UINT32				OutputLength = 0;
	PIO_STACK_LOCATION	IrpStack;
	UINT32				IoControlCode;

	IrpStack = IoGetCurrentIrpStackLocation(Irp);		// 获得当前Irp堆栈
	InputBuffer = IrpStack->Parameters.DeviceIoControl.Type3InputBuffer;
	OutputBuffer = Irp->UserBuffer;
	InputLength = IrpStack->Parameters.DeviceIoControl.InputBufferLength;
	OutputLength = IrpStack->Parameters.DeviceIoControl.OutputBufferLength;

	Irp->IoStatus.Status = STATUS_SUCCESS;
	Irp->IoStatus.Information = 0;

	switch (IrpStack->MajorFunction)
	{
	case IRP_MJ_DEVICE_CONTROL:
	{
		IoControlCode = IrpStack->Parameters.DeviceIoControl.IoControlCode;
		switch (IoControlCode)
		{
			//
			// ProcessCore
			//

		case IOCTL_ARKPROTECT_PROCESSNUM:
		{
			DbgPrint("Get Process Count\r\n");

			if (OutputBuffer)
			{
				__try
				{
					ProbeForWrite(OutputBuffer, OutputLength, sizeof(UINT32));

					Status = APGetProcessNum(OutputBuffer);

					Irp->IoStatus.Status = Status;
				}
				__except (EXCEPTION_EXECUTE_HANDLER)
				{
					DbgPrint("Catch Exception\r\n");
					Status = STATUS_UNSUCCESSFUL;
				}
			}
			else
			{
				Irp->IoStatus.Status = STATUS_INVALID_PARAMETER;
			}

			break;
		}
		case IOCTL_ARKPROTECT_ENUMPROCESS:
		{
			DbgPrint("Enum Process\r\n");

			if (OutputBuffer)
			{
				__try
				{
					ProbeForWrite(OutputBuffer, OutputLength, sizeof(UINT8));

					Status = APEnumProcessInfo(OutputBuffer, OutputLength);

					Irp->IoStatus.Status = Status;
				}
				__except (EXCEPTION_EXECUTE_HANDLER)
				{
					DbgPrint("Catch Exception\r\n");
					Status = STATUS_UNSUCCESSFUL;
				}
			}
			else
			{
				Irp->IoStatus.Status = STATUS_INVALID_PARAMETER;
			}

			break;
		}
		case IOCTL_ARKPROTECT_ENUMPROCESSMODULE:
		{
			DbgPrint("Process Module\r\n");

			if (InputLength >= sizeof(UINT32) && InputBuffer && OutputBuffer)
			{
				__try
				{
					ProbeForRead(InputBuffer, InputLength, sizeof(UINT32));
					ProbeForWrite(OutputBuffer, OutputLength, sizeof(UINT8));

					Status = APEnumProcessModule(*(PUINT32)InputBuffer, OutputBuffer, OutputLength);

					Irp->IoStatus.Status = Status;
				}
				__except (EXCEPTION_EXECUTE_HANDLER)
				{
					DbgPrint("Catch Exception\r\n");
					Status = STATUS_UNSUCCESSFUL;
				}
			}
			else
			{
				Irp->IoStatus.Status = STATUS_INFO_LENGTH_MISMATCH;
			}

			break;
		}
		case IOCTL_ARKPROTECT_ENUMPROCESSTHREAD:
		{
			DbgPrint("Process Thread\r\n");

			if (InputLength >= sizeof(UINT32) && InputBuffer && OutputBuffer)
			{
				__try
				{
					ProbeForRead(InputBuffer, InputLength, sizeof(UINT32));
					ProbeForWrite(OutputBuffer, OutputLength, sizeof(UINT8));

					Status = APEnumProcessThread(*(PUINT32)InputBuffer, OutputBuffer, OutputLength);

					Irp->IoStatus.Status = Status;
				}
				__except (EXCEPTION_EXECUTE_HANDLER)
				{
					DbgPrint("Catch Exception\r\n");
					Status = STATUS_UNSUCCESSFUL;
				}
			}
			else
			{
				Irp->IoStatus.Status = STATUS_INFO_LENGTH_MISMATCH;
			}

			break;
		}
		case IOCTL_ARKPROTECT_ENUMPROCESSHANDLE:
		{
			DbgPrint("Process Handle\r\n");

			if (InputLength >= sizeof(UINT32) && InputBuffer && OutputBuffer)
			{
				__try
				{
					ProbeForRead(InputBuffer, InputLength, sizeof(UINT32));
					ProbeForWrite(OutputBuffer, OutputLength, sizeof(UINT8));

					Status = APEnumProcessHandle(*(PUINT32)InputBuffer, OutputBuffer, OutputLength);

					Irp->IoStatus.Status = Status;
				}
				__except (EXCEPTION_EXECUTE_HANDLER)
				{
					DbgPrint("Catch Exception\r\n");
					Status = STATUS_UNSUCCESSFUL;
				}
			}
			else
			{
				Irp->IoStatus.Status = STATUS_INFO_LENGTH_MISMATCH;
			}

			break;
		}
		case IOCTL_ARKPROTECT_ENUMPROCESSWINDOW:
		{
			DbgPrint("Process Window\r\n");

			if (InputLength >= sizeof(UINT32) && InputBuffer && OutputBuffer)
			{
				__try
				{
					ProbeForRead(InputBuffer, InputLength, sizeof(UINT32));
					ProbeForWrite(OutputBuffer, OutputLength, sizeof(UINT8));

					Status = APEnumProcessWindow(*(PUINT32)InputBuffer, OutputBuffer, OutputLength);

					Irp->IoStatus.Status = Status;
				}
				__except (EXCEPTION_EXECUTE_HANDLER)
				{
					DbgPrint("Catch Exception\r\n");
					Status = STATUS_UNSUCCESSFUL;
				}
			}
			else
			{
				Irp->IoStatus.Status = STATUS_INFO_LENGTH_MISMATCH;
			}

			break;
		}
		case IOCTL_ARKPROTECT_ENUMPROCESSMEMORY:
		{
			DbgPrint("Process Memory\r\n");

			if (InputLength >= sizeof(UINT32) && InputBuffer && OutputBuffer)
			{
				__try
				{
					ProbeForRead(InputBuffer, InputLength, sizeof(UINT32));
					ProbeForWrite(OutputBuffer, OutputLength, sizeof(UINT8));

					Status = APEnumProcessMemory(*(PUINT32)InputBuffer, OutputBuffer, OutputLength);

					Irp->IoStatus.Status = Status;
				}
				__except (EXCEPTION_EXECUTE_HANDLER)
				{
					DbgPrint("Catch Exception\r\n");
					Status = STATUS_UNSUCCESSFUL;
				}
			}
			else
			{
				Irp->IoStatus.Status = STATUS_INFO_LENGTH_MISMATCH;
			}

			break;
		}
		case IOCTL_ARKPROTECT_TERMINATEPROCESS:
		{
			DbgPrint("Terminate Process\r\n");

			if (InputLength >= sizeof(UINT32) && InputBuffer)
			{
				__try
				{
					ProbeForRead(InputBuffer, InputLength, sizeof(UINT32));

					Status = APTerminateProcess(*(PUINT32)InputBuffer);

					Irp->IoStatus.Status = Status;
				}
				__except (EXCEPTION_EXECUTE_HANDLER)
				{
					DbgPrint("Catch Exception\r\n");
					Status = STATUS_UNSUCCESSFUL;
				}
			}
			else
			{
				Irp->IoStatus.Status = STATUS_INFO_LENGTH_MISMATCH;
			}

			break;
		}

		// 
		// Driver
		// 

		case IOCTL_ARKPROTECT_ENUMDRIVER:
		{
			DbgPrint("Enum Driver\r\n");

			if (OutputBuffer)
			{
				__try
				{
					ProbeForWrite(OutputBuffer, OutputLength, sizeof(UINT8));

					Status = APEnumDriverInfo(OutputBuffer, OutputLength);

					Irp->IoStatus.Status = Status;
				}
				__except (EXCEPTION_EXECUTE_HANDLER)
				{
					DbgPrint("Catch Exception\r\n");
					Status = STATUS_UNSUCCESSFUL;
				}
			}
			else
			{
				Irp->IoStatus.Status = STATUS_INVALID_PARAMETER;
			}

			break;
		}
		case IOCTL_ARKPROTECT_UNLOADRIVER:
		{
			DbgPrint("Unload Driver\r\n");

			if (InputLength >= sizeof(UINT_PTR) && InputBuffer)
			{
				__try
				{
					ProbeForRead(InputBuffer, InputLength, sizeof(UINT_PTR));

					Status = APUnloadDriverObject(*(PUINT_PTR)InputBuffer);

					Irp->IoStatus.Status = Status;
				}
				__except (EXCEPTION_EXECUTE_HANDLER)
				{
					DbgPrint("Catch Exception\r\n");
					Status = STATUS_UNSUCCESSFUL;
				}
			}
			else
			{
				Irp->IoStatus.Status = STATUS_INFO_LENGTH_MISMATCH;
			}

			break;
		}

		//
		// KernelCore
		//
		case IOCTL_ARKPROTECT_ENUMSYSCALLBACK:
		{
			DbgPrint("Enum Callback\r\n");

			if (OutputBuffer)
			{
				__try
				{
					ProbeForWrite(OutputBuffer, OutputLength, sizeof(UINT8));

					Status = APEnumSystemCallback(OutputBuffer, OutputLength);

					Irp->IoStatus.Status = Status;
				}
				__except (EXCEPTION_EXECUTE_HANDLER)
				{
					DbgPrint("Catch Exception\r\n");
					Status = STATUS_UNSUCCESSFUL;
				}
			}
			else
			{
				Irp->IoStatus.Status = STATUS_INVALID_PARAMETER;
			}

			break;
		}
		case IOCTL_ARKPROTECT_ENUMFILTERDRIVER:
		{
			DbgPrint("Enum FilterDriver\r\n");

			if (OutputBuffer)
			{
				__try
				{
					ProbeForWrite(OutputBuffer, OutputLength, sizeof(UINT8));

					Status = APEnumFilterDriver(OutputBuffer, OutputLength);

					Irp->IoStatus.Status = Status;
				}
				__except (EXCEPTION_EXECUTE_HANDLER)
				{
					DbgPrint("Catch Exception\r\n");
					Status = STATUS_UNSUCCESSFUL;
				}
			}
			else
			{
				Irp->IoStatus.Status = STATUS_INVALID_PARAMETER;
			}

			break;
		}
		case IOCTL_ARKPROTECT_ENUMDPCTIMER:
		{
			DbgPrint("Enum DpcTimer\r\n");

			if (OutputBuffer)
			{
				__try
				{
					ProbeForWrite(OutputBuffer, OutputLength, sizeof(UINT8));

					Status = APEnumDpcTimer(OutputBuffer, OutputLength);

					Irp->IoStatus.Status = Status;
				}
				__except (EXCEPTION_EXECUTE_HANDLER)
				{
					DbgPrint("Catch Exception\r\n");
					Status = STATUS_UNSUCCESSFUL;
				}
			}
			else
			{
				Irp->IoStatus.Status = STATUS_INVALID_PARAMETER;
			}

			break;
		}
		case IOCTL_ARKPROTECT_ENUMIOTIMER:
		{
			DbgPrint("Enum IoTimer\r\n");

			if (OutputBuffer)
			{
				__try
				{
					ProbeForWrite(OutputBuffer, OutputLength, sizeof(UINT8));

					Status = APEnumIoTimer(OutputBuffer, OutputLength);

					Irp->IoStatus.Status = Status;
				}
				__except (EXCEPTION_EXECUTE_HANDLER)
				{
					DbgPrint("Catch Exception\r\n");
					Status = STATUS_UNSUCCESSFUL;
				}
			}
			else
			{
				Irp->IoStatus.Status = STATUS_INVALID_PARAMETER;
			}

			break;
		}
		case IOCTL_ARKPROTECT_ENUMSSDTHOOK:
		{
			DbgPrint("Enum SsdtHook\r\n");

			if (OutputBuffer)
			{
				__try
				{
					ProbeForWrite(OutputBuffer, OutputLength, sizeof(UINT8));

					Status = APEnumSsdtHook(OutputBuffer, OutputLength);

					Irp->IoStatus.Status = Status;
				}
				__except (EXCEPTION_EXECUTE_HANDLER)
				{
					DbgPrint("Catch Exception\r\n");
					Status = STATUS_UNSUCCESSFUL;
				}
			}
			else
			{
				Irp->IoStatus.Status = STATUS_INVALID_PARAMETER;
			}

			break;
		}

		default:
			Irp->IoStatus.Status = STATUS_INVALID_PARAMETER;
			break;
		}
	}
	default:
		break;
	}

	Status = Irp->IoStatus.Status;
	IoCompleteRequest(Irp, IO_NO_INCREMENT);

	return Status;
}


