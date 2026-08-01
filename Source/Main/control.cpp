/*
 * AudioNT additions are licensed under the MIT License.
 * See the repository LICENSE and THIRD_PARTY_NOTICES.md files.
 */

#include "control.h"

#include <wdmsec.h>

#include "pcmbridge.h"
#include "../Shared/audiont_control_protocol.h"

namespace
{
const GUID AudioNtControlClassGuid =
{
    0x902ff38d,
    0xd7b0,
    0x4f55,
    {0xa4, 0x83, 0x76, 0x8d, 0x2d, 0xad, 0x84, 0xc9},
};

PDEVICE_OBJECT g_AudioNtControlDevice = nullptr;
PDRIVER_DISPATCH g_PortClsCreateDispatch = nullptr;
PDRIVER_DISPATCH g_PortClsCloseDispatch = nullptr;
PDRIVER_DISPATCH g_PortClsDeviceControlDispatch = nullptr;
UNICODE_STRING g_AudioNtControlSymbolicLink{};

NTSTATUS CompleteRequest(
    _Inout_ PIRP irp,
    NTSTATUS status,
    ULONG_PTR information = 0)
{
    irp->IoStatus.Status = status;
    irp->IoStatus.Information = information;
    IoCompleteRequest(irp, IO_NO_INCREMENT);
    return status;
}

NTSTATUS HandleDeviceControl(_Inout_ PIRP irp)
{
    const auto stack = IoGetCurrentIrpStackLocation(irp);
    const ULONG controlCode = stack->Parameters.DeviceIoControl.IoControlCode;
    const ULONG inputLength = stack->Parameters.DeviceIoControl.InputBufferLength;
    const ULONG outputLength = stack->Parameters.DeviceIoControl.OutputBufferLength;
    void* const buffer = irp->AssociatedIrp.SystemBuffer;

    switch (controlCode)
    {
    case IOCTL_AUDIONT_QUERY_CAPABILITIES:
    {
        if (buffer == nullptr || outputLength < sizeof(AudioNtControlCapabilities))
        {
            return CompleteRequest(irp, STATUS_BUFFER_TOO_SMALL);
        }
        auto* capabilities = static_cast<AudioNtControlCapabilities*>(buffer);
        *capabilities =
        {
            sizeof(AudioNtControlCapabilities),
            AUDIONT_CONTROL_PROTOCOL_VERSION,
            AUDIONT_MIC_SAMPLE_RATE,
            AUDIONT_MIC_CHANNELS,
            AUDIONT_MIC_BITS_PER_SAMPLE,
            AUDIONT_MIC_MAX_FRAMES_PER_WRITE,
        };
        return CompleteRequest(
            irp,
            STATUS_SUCCESS,
            sizeof(AudioNtControlCapabilities));
    }
    case IOCTL_AUDIONT_RESET_MICROPHONE:
        AudioNtMicrophoneBridgeReset();
        return CompleteRequest(irp, STATUS_SUCCESS);

    case IOCTL_AUDIONT_WRITE_MICROPHONE:
    {
        const auto* header =
            inputLength >= sizeof(AudioNtMicrophoneWriteHeader)
                ? static_cast<const AudioNtMicrophoneWriteHeader*>(buffer)
                : nullptr;
        const AudioNtProtocolValidation validation =
            AudioNtValidateMicrophoneWrite(header, inputLength);
        if (validation != AudioNtProtocolValidation::Valid)
        {
            const NTSTATUS status =
                validation == AudioNtProtocolValidation::UnsupportedVersion
                    ? STATUS_REVISION_MISMATCH
                    : STATUS_INVALID_PARAMETER;
            return CompleteRequest(irp, status);
        }

        const auto* payload =
            reinterpret_cast<const BYTE*>(header) + sizeof(*header);
        AudioNtMicrophoneBridgeWrite(
            payload,
            header->PayloadBytes,
            header->Sequence);
        return CompleteRequest(irp, STATUS_SUCCESS);
    }
    case IOCTL_AUDIONT_GET_MICROPHONE_STATS:
    {
        if (buffer == nullptr || outputLength < sizeof(AudioNtMicrophoneStats))
        {
            return CompleteRequest(irp, STATUS_BUFFER_TOO_SMALL);
        }
        auto* stats = static_cast<AudioNtMicrophoneStats*>(buffer);
        AudioNtMicrophoneBridgeGetStats(stats);
        return CompleteRequest(
            irp,
            STATUS_SUCCESS,
            sizeof(AudioNtMicrophoneStats));
    }
    default:
        return CompleteRequest(irp, STATUS_INVALID_DEVICE_REQUEST);
    }
}

NTSTATUS AudioNtControlDispatch(
    _In_ PDEVICE_OBJECT deviceObject,
    _Inout_ PIRP irp)
{
    const auto stack = IoGetCurrentIrpStackLocation(irp);
    if (deviceObject != g_AudioNtControlDevice)
    {
        PDRIVER_DISPATCH dispatch = nullptr;
        switch (stack->MajorFunction)
        {
        case IRP_MJ_CREATE:
            dispatch = g_PortClsCreateDispatch;
            break;
        case IRP_MJ_CLOSE:
            dispatch = g_PortClsCloseDispatch;
            break;
        case IRP_MJ_DEVICE_CONTROL:
            dispatch = g_PortClsDeviceControlDispatch;
            break;
        default:
            break;
        }
        return dispatch == nullptr
            ? CompleteRequest(irp, STATUS_INVALID_DEVICE_REQUEST)
            : dispatch(deviceObject, irp);
    }

    switch (stack->MajorFunction)
    {
    case IRP_MJ_CREATE:
    case IRP_MJ_CLOSE:
        return CompleteRequest(irp, STATUS_SUCCESS);
    case IRP_MJ_DEVICE_CONTROL:
        return HandleDeviceControl(irp);
    default:
        return CompleteRequest(irp, STATUS_INVALID_DEVICE_REQUEST);
    }
}
}

#pragma code_seg("INIT")
NTSTATUS AudioNtControlInitialize(PDRIVER_OBJECT driverObject)
{
    if (driverObject == nullptr || g_AudioNtControlDevice != nullptr)
    {
        return STATUS_INVALID_PARAMETER;
    }

    UNICODE_STRING deviceName;
    UNICODE_STRING securityDescriptor;
    RtlInitUnicodeString(&deviceName, AUDIONT_CONTROL_NT_DEVICE_NAME);
    RtlInitUnicodeString(
        &securityDescriptor,
        L"D:P(A;;GA;;;SY)(A;;GA;;;BA)");
    RtlInitUnicodeString(
        &g_AudioNtControlSymbolicLink,
        AUDIONT_CONTROL_DOS_DEVICE_NAME);

    NTSTATUS status = IoCreateDeviceSecure(
        driverObject,
        0,
        &deviceName,
        FILE_DEVICE_SOUND,
        FILE_DEVICE_SECURE_OPEN,
        TRUE,
        &securityDescriptor,
        &AudioNtControlClassGuid,
        &g_AudioNtControlDevice);
    if (!NT_SUCCESS(status))
    {
        g_AudioNtControlDevice = nullptr;
        return status;
    }

    status = IoCreateSymbolicLink(
        &g_AudioNtControlSymbolicLink,
        &deviceName);
    if (!NT_SUCCESS(status))
    {
        IoDeleteDevice(g_AudioNtControlDevice);
        g_AudioNtControlDevice = nullptr;
        return status;
    }

    g_PortClsCreateDispatch = driverObject->MajorFunction[IRP_MJ_CREATE];
    g_PortClsCloseDispatch = driverObject->MajorFunction[IRP_MJ_CLOSE];
    g_PortClsDeviceControlDispatch =
        driverObject->MajorFunction[IRP_MJ_DEVICE_CONTROL];
    driverObject->MajorFunction[IRP_MJ_CREATE] = AudioNtControlDispatch;
    driverObject->MajorFunction[IRP_MJ_CLOSE] = AudioNtControlDispatch;
    driverObject->MajorFunction[IRP_MJ_DEVICE_CONTROL] = AudioNtControlDispatch;

    g_AudioNtControlDevice->Flags |= DO_BUFFERED_IO;
    g_AudioNtControlDevice->Flags &= ~DO_DEVICE_INITIALIZING;
    return STATUS_SUCCESS;
}

#pragma code_seg("PAGE")
void AudioNtControlShutdown(PDRIVER_OBJECT driverObject)
{
    PAGED_CODE();
    if (driverObject != nullptr)
    {
        if (g_PortClsCreateDispatch != nullptr)
        {
            driverObject->MajorFunction[IRP_MJ_CREATE] = g_PortClsCreateDispatch;
        }
        if (g_PortClsCloseDispatch != nullptr)
        {
            driverObject->MajorFunction[IRP_MJ_CLOSE] = g_PortClsCloseDispatch;
        }
        if (g_PortClsDeviceControlDispatch != nullptr)
        {
            driverObject->MajorFunction[IRP_MJ_DEVICE_CONTROL] =
                g_PortClsDeviceControlDispatch;
        }
    }

    if (g_AudioNtControlDevice != nullptr)
    {
        IoDeleteSymbolicLink(&g_AudioNtControlSymbolicLink);
        IoDeleteDevice(g_AudioNtControlDevice);
        g_AudioNtControlDevice = nullptr;
    }
    g_PortClsCreateDispatch = nullptr;
    g_PortClsCloseDispatch = nullptr;
    g_PortClsDeviceControlDispatch = nullptr;
}
