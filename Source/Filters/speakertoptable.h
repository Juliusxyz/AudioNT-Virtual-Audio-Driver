/*++

Copyright (c) Microsoft Corporation All Rights Reserved

Module Name:

    speakertoptable.h

Abstract:

    Declaration of topology tables.
--*/

#ifndef _VIRTUALAUDIODRIVER_SPEAKERTOPTABLE_H_
#define _VIRTUALAUDIODRIVER_SPEAKERTOPTABLE_H_

// Device-specific pin names let each fixed AudioNT render bus expose its own
// stable name while retaining the standard Windows speaker endpoint category.
DEFINE_GUID(AUDIONT_GAME_ENDPOINT_CATEGORY,
    0xd4be6922, 0xc928, 0x421b, 0xa0, 0xf3, 0x45, 0x80, 0x10, 0x6e, 0xa6, 0x7d);
DEFINE_GUID(AUDIONT_CHAT_ENDPOINT_CATEGORY,
    0xb4e176ec, 0xd673, 0x4431, 0x88, 0xb1, 0x16, 0x5c, 0x9f, 0xdf, 0x04, 0xaf);
DEFINE_GUID(AUDIONT_MEDIA_ENDPOINT_CATEGORY,
    0xbcf8d816, 0x5786, 0x47ee, 0x90, 0x96, 0x46, 0xa4, 0x0c, 0x74, 0x25, 0xc8);
DEFINE_GUID(AUDIONT_AUX_ENDPOINT_CATEGORY,
    0x260dd4b5, 0xcc8d, 0x478b, 0xa1, 0x50, 0x0b, 0x9c, 0x28, 0x02, 0x2b, 0xa5);

//=============================================================================
static
KSDATARANGE SpeakerTopoPinDataRangesBridge[] =
{
 {
   sizeof(KSDATARANGE),
   0,
   0,
   0,
   STATICGUIDOF(KSDATAFORMAT_TYPE_AUDIO),
   STATICGUIDOF(KSDATAFORMAT_SUBTYPE_ANALOG),
   STATICGUIDOF(KSDATAFORMAT_SPECIFIER_NONE)
 }
};

//=============================================================================
static
PKSDATARANGE SpeakerTopoPinDataRangePointersBridge[] =
{
  &SpeakerTopoPinDataRangesBridge[0]
};

//=============================================================================
static
KSJACK_DESCRIPTION SpeakerJackDescBridge =
{
    KSAUDIO_SPEAKER_STEREO,
    JACKDESC_RGB(0xB3,0xC9,0x8C),              // Color spec for green
    eConnTypeUnknown,
    eGeoLocFront,
    eGenLocPrimaryBox,
    ePortConnIntegratedDevice,
    TRUE
};

// Only return a KSJACK_DESCRIPTION for the physical bridge pin.
static 
PKSJACK_DESCRIPTION SpeakerJackDescriptions[] =
{
    NULL,
    &SpeakerJackDescBridge
};

//=============================================================================
static
PCPROPERTY_ITEM SpeakerPropertiesVolume[] =
{
    {
    &KSPROPSETID_Audio,
    KSPROPERTY_AUDIO_VOLUMELEVEL,
    KSPROPERTY_TYPE_GET | KSPROPERTY_TYPE_SET | KSPROPERTY_TYPE_BASICSUPPORT,
    PropertyHandler_SpeakerTopology
    }
};

DEFINE_PCAUTOMATION_TABLE_PROP(AutomationSpeakerVolume, SpeakerPropertiesVolume);

//=============================================================================
static
PCPROPERTY_ITEM SpeakerPropertiesMute[] =
{
  {
    &KSPROPSETID_Audio,
    KSPROPERTY_AUDIO_MUTE,
    KSPROPERTY_TYPE_GET | KSPROPERTY_TYPE_SET | KSPROPERTY_TYPE_BASICSUPPORT,
    PropertyHandler_SpeakerTopology
  }
};

DEFINE_PCAUTOMATION_TABLE_PROP(AutomationSpeakerMute, SpeakerPropertiesMute);

//=============================================================================
static
PCNODE_DESCRIPTOR SpeakerTopologyNodes[] =
{
    // KSNODE_TOPO_VOLUME
    {
      0,                              // Flags
      &AutomationSpeakerVolume,     // AutomationTable
      &KSNODETYPE_VOLUME,             // Type
      &KSAUDFNAME_MASTER_VOLUME         // Name
    },
    // KSNODE_TOPO_MUTE
    {
      0,                              // Flags
      &AutomationSpeakerMute,       // AutomationTable
      &KSNODETYPE_MUTE,               // Type
      &KSAUDFNAME_MASTER_MUTE            // Name
    }
};

C_ASSERT(KSNODE_TOPO_VOLUME == 0);
C_ASSERT(KSNODE_TOPO_MUTE == 1);

static
PCCONNECTION_DESCRIPTOR SpeakerTopoMiniportConnections[] =
{
    //  FromNode,                 FromPin,                    ToNode,                 ToPin
    {   PCFILTER_NODE,            KSPIN_TOPO_WAVEOUT_SOURCE,    KSNODE_TOPO_VOLUME,     1 },
    {   KSNODE_TOPO_VOLUME,       0,                          KSNODE_TOPO_MUTE,       1 },
    {   KSNODE_TOPO_MUTE,         0,                          PCFILTER_NODE,          KSPIN_TOPO_LINEOUT_DEST }
};

//=============================================================================
static
PCPROPERTY_ITEM PropertiesSpeakerTopoFilter[] =
{
    {
        &KSPROPSETID_Jack,
        KSPROPERTY_JACK_DESCRIPTION,
        KSPROPERTY_TYPE_GET |
        KSPROPERTY_TYPE_BASICSUPPORT,
        PropertyHandler_SpeakerTopoFilter
    },
    {
        &KSPROPSETID_Jack,
        KSPROPERTY_JACK_DESCRIPTION2,
        KSPROPERTY_TYPE_GET |
        KSPROPERTY_TYPE_BASICSUPPORT,
        PropertyHandler_SpeakerTopoFilter
    }
};

DEFINE_PCAUTOMATION_TABLE_PROP(AutomationSpeakerTopoFilter, PropertiesSpeakerTopoFilter);

//=============================================================================
#define AUDIONT_RENDER_TOPOLOGY_DESCRIPTOR(symbol, endpointName)                     \
static PCPIN_DESCRIPTOR symbol##SpeakerTopoMiniportPins[] =                          \
{                                                                                    \
  {                                                                                  \
    0, 0, 0, NULL,                                                                   \
    {                                                                                \
      0, NULL, 0, NULL,                                                              \
      SIZEOF_ARRAY(SpeakerTopoPinDataRangePointersBridge),                           \
      SpeakerTopoPinDataRangePointersBridge,                                         \
      KSPIN_DATAFLOW_IN, KSPIN_COMMUNICATION_NONE,                                   \
      &KSCATEGORY_AUDIO, NULL, 0                                                     \
    }                                                                                \
  },                                                                                 \
  {                                                                                  \
    0, 0, 0, NULL,                                                                   \
    {                                                                                \
      0, NULL, 0, NULL,                                                              \
      SIZEOF_ARRAY(SpeakerTopoPinDataRangePointersBridge),                           \
      SpeakerTopoPinDataRangePointersBridge,                                         \
      KSPIN_DATAFLOW_OUT, KSPIN_COMMUNICATION_NONE,                                  \
      &KSNODETYPE_SPEAKER, &endpointName, 0                                          \
    }                                                                                \
  }                                                                                  \
};                                                                                   \
static PCFILTER_DESCRIPTOR symbol##SpeakerTopoMiniportFilterDescriptor =             \
{                                                                                    \
  0,                                                                                 \
  &AutomationSpeakerTopoFilter,                                                      \
  sizeof(PCPIN_DESCRIPTOR),                                                          \
  SIZEOF_ARRAY(symbol##SpeakerTopoMiniportPins),                                      \
  symbol##SpeakerTopoMiniportPins,                                                    \
  sizeof(PCNODE_DESCRIPTOR),                                                         \
  SIZEOF_ARRAY(SpeakerTopologyNodes),                                                \
  SpeakerTopologyNodes,                                                              \
  SIZEOF_ARRAY(SpeakerTopoMiniportConnections),                                      \
  SpeakerTopoMiniportConnections,                                                    \
  0,                                                                                 \
  NULL                                                                               \
}

AUDIONT_RENDER_TOPOLOGY_DESCRIPTOR(Game, AUDIONT_GAME_ENDPOINT_CATEGORY);
AUDIONT_RENDER_TOPOLOGY_DESCRIPTOR(Chat, AUDIONT_CHAT_ENDPOINT_CATEGORY);
AUDIONT_RENDER_TOPOLOGY_DESCRIPTOR(Media, AUDIONT_MEDIA_ENDPOINT_CATEGORY);
AUDIONT_RENDER_TOPOLOGY_DESCRIPTOR(Aux, AUDIONT_AUX_ENDPOINT_CATEGORY);

#undef AUDIONT_RENDER_TOPOLOGY_DESCRIPTOR

#endif // _VIRTUALAUDIODRIVER_SPEAKERTOPTABLE_H_
