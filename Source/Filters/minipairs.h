/*++

Copyright (c) Microsoft Corporation All Rights Reserved

Module Name:

    minipairs.h

Abstract:

    Local audio endpoint filter definitions. 
--*/

#ifndef _VIRTUALAUDIODRIVER_MINIPAIRS_H_
#define _VIRTUALAUDIODRIVER_MINIPAIRS_H_

#include "speakertopo.h"
#include "speakertoptable.h"
#include "speakerwavtable.h"

#include "micarraytopo.h"
#include "micarray1toptable.h"
#include "micarraywavtable.h"


NTSTATUS
CreateMiniportWaveRTVirtualAudioDriver
( 
    _Out_       PUNKNOWN *,
    _In_        REFCLSID,
    _In_opt_    PUNKNOWN,
    _In_        POOL_FLAGS,
    _In_        PUNKNOWN,
    _In_opt_    PVOID,
    _In_        PENDPOINT_MINIPAIR
);

NTSTATUS
CreateMiniportTopologyVirtualAudioDriver
( 
    _Out_       PUNKNOWN *,
    _In_        REFCLSID,
    _In_opt_    PUNKNOWN,
    _In_        POOL_FLAGS,
    _In_        PUNKNOWN,
    _In_opt_    PVOID,
    _In_        PENDPOINT_MINIPAIR
);

//
// Render miniports.
//

/*********************************************************************
* Topology/Wave bridge connection for speaker (internal)             *
*                                                                    *
*              +------+                +------+                      *
*              | Wave |                | Topo |                      *
*              |      |                |      |                      *
* System   --->|0    1|--------------->|0    1|---> Line Out         *
*              |      |                |      |                      *
*              +------+                +------+                      *
*********************************************************************/
static
PHYSICALCONNECTIONTABLE SpeakerTopologyPhysicalConnections[] =
{
    {
        KSPIN_TOPO_WAVEOUT_SOURCE,  // TopologyIn
        KSPIN_WAVE_RENDER3_SOURCE,   // WaveOut
        CONNECTIONTYPE_WAVE_OUTPUT
    }
};

#define AUDIONT_RENDER_INTERFACE_PROPERTIES(endpoint, friendlyName)  \
static WCHAR endpoint##RenderInterfaceFriendlyName[] = friendlyName; \
static const VIRTUALAUDIODRIVER_DEVPROPERTY endpoint##RenderInterfaceProperties[] = \
{                                                                     \
    {                                                                 \
        &DEVPKEY_DeviceInterface_FriendlyName,                         \
        DEVPROP_TYPE_STRING_INDIRECT,                                 \
        sizeof(endpoint##RenderInterfaceFriendlyName),                \
        endpoint##RenderInterfaceFriendlyName                         \
    }                                                                 \
}

AUDIONT_RENDER_INTERFACE_PROPERTIES(Game, L"AudioNT - Game");
AUDIONT_RENDER_INTERFACE_PROPERTIES(Chat, L"AudioNT - Chat");
AUDIONT_RENDER_INTERFACE_PROPERTIES(Media, L"AudioNT - Media");
AUDIONT_RENDER_INTERFACE_PROPERTIES(Aux, L"AudioNT - AUX");

#define AUDIONT_RENDER_MINIPAIR(symbol, endpoint, deviceType, topologyName, topologyDescriptor, waveName) \
static ENDPOINT_MINIPAIR symbol =                                      \
{                                                                      \
    deviceType,                                                        \
    topologyName,                                                      \
    NULL,                                                              \
    CreateMiniportTopologyVirtualAudioDriver,                          \
    topologyDescriptor,                                                \
    SIZEOF_ARRAY(endpoint##RenderInterfaceProperties),                 \
    endpoint##RenderInterfaceProperties,                               \
    waveName,                                                          \
    NULL,                                                              \
    CreateMiniportWaveRTVirtualAudioDriver,                            \
    &SpeakerWaveMiniportFilterDescriptor,                              \
    SIZEOF_ARRAY(endpoint##RenderInterfaceProperties),                 \
    endpoint##RenderInterfaceProperties,                               \
    SPEAKER_DEVICE_MAX_CHANNELS,                                       \
    SpeakerPinDeviceFormatsAndModes,                                   \
    SIZEOF_ARRAY(SpeakerPinDeviceFormatsAndModes),                     \
    SpeakerTopologyPhysicalConnections,                               \
    SIZEOF_ARRAY(SpeakerTopologyPhysicalConnections),                  \
    ENDPOINT_NO_FLAGS,                                                 \
}

AUDIONT_RENDER_MINIPAIR(
    GameMiniports,
    Game,
    eAudioNtGameDevice,
    L"AudioNTTopologyGame",
    &GameSpeakerTopoMiniportFilterDescriptor,
    L"AudioNTWaveGame");

AUDIONT_RENDER_MINIPAIR(
    ChatMiniports,
    Chat,
    eAudioNtChatDevice,
    L"AudioNTTopologyChat",
    &ChatSpeakerTopoMiniportFilterDescriptor,
    L"AudioNTWaveChat");

AUDIONT_RENDER_MINIPAIR(
    MediaMiniports,
    Media,
    eAudioNtMediaDevice,
    L"AudioNTTopologyMedia",
    &MediaSpeakerTopoMiniportFilterDescriptor,
    L"AudioNTWaveMedia");

AUDIONT_RENDER_MINIPAIR(
    AuxMiniports,
    Aux,
    eAudioNtAuxDevice,
    L"AudioNTTopologyAux",
    &AuxSpeakerTopoMiniportFilterDescriptor,
    L"AudioNTWaveAux");

#undef AUDIONT_RENDER_MINIPAIR

//
// Capture miniports.
//

/*********************************************************************
* Topology/Wave bridge connection for mic array  1 (front)           *
*                                                                    *
*              +------+    +------+                                  *
*              | Topo |    | Wave |                                  *
*              |      |    |      |                                  *
*  Mic in  --->|0    1|===>|0    1|---> Capture Host Pin             *
*              |      |    |      |                                  *
*              +------+    +------+                                  *
*********************************************************************/
static
PHYSICALCONNECTIONTABLE MicArray1TopologyPhysicalConnections[] =
{
    {
        KSPIN_TOPO_BRIDGE,          // TopologyOut
        KSPIN_WAVE_BRIDGE,          // WaveIn
        CONNECTIONTYPE_TOPOLOGY_OUTPUT
    }
};

static
ENDPOINT_MINIPAIR MicrophoneMiniports =
{
    eAudioNtMicrophoneDevice,
    L"AudioNTTopologyMicrophone",
    NULL,                                   // optional template name
    CreateMicArrayMiniportTopology,
    &MicArray1TopoMiniportFilterDescriptor,
    0, NULL,                                // Interface properties
    L"AudioNTWaveMicrophone",
    NULL,                                   // optional template name
    CreateMiniportWaveRTVirtualAudioDriver,
    &MicArrayWaveMiniportFilterDescriptor,
    0,                                      // Interface properties
    NULL,
    MICARRAY_DEVICE_MAX_CHANNELS,
    MicArrayPinDeviceFormatsAndModes,
    SIZEOF_ARRAY(MicArrayPinDeviceFormatsAndModes),
    MicArray1TopologyPhysicalConnections,
    SIZEOF_ARRAY(MicArray1TopologyPhysicalConnections),
    ENDPOINT_NO_FLAGS,
};


//=============================================================================
//
// Render miniport pairs. NOTE: the split of render and capture is arbitrary and
// unnessary, this array could contain capture endpoints.
//
static
PENDPOINT_MINIPAIR  g_RenderEndpoints[] = 
{
    &GameMiniports,
    &ChatMiniports,
    &MediaMiniports,
    &AuxMiniports,
};

#define g_cRenderEndpoints  (SIZEOF_ARRAY(g_RenderEndpoints))

//=============================================================================
//
// Capture miniport pairs. NOTE: the split of render and capture is arbitrary and
// unnessary, this array could contain render endpoints.
//
static
PENDPOINT_MINIPAIR  g_CaptureEndpoints[] =
{
    &MicrophoneMiniports,
};

#define g_cCaptureEndpoints (SIZEOF_ARRAY(g_CaptureEndpoints))

//=============================================================================
//
// Total miniports = # endpoints * 2 (topology + wave).
//
#define g_MaxMiniports  ((g_cRenderEndpoints + g_cCaptureEndpoints) * 2)

#endif // _VIRTUALAUDIODRIVER_MINIPAIRS_H_
