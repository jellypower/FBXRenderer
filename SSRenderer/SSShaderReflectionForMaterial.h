#pragma once

#define SHADER_FILE_NAME_MAX_LEN 128
#define VS_SHADER_ENTRY_NAME_MAX_LEN 32
#define PS_SHADER_ENTRY_NAME_MAX_LEN 32
#define LAYOUT_SEMANTIC_NAME_LEN_MAX 16

#define LAYOUT_NUM_MAX D3D11_IA_VERTEX_INPUT_RESOURCE_SLOT_COUNT 
#define CONSTANT_BUFFER_COUNT_MAX D3D11_COMMONSHADER_CONSTANT_BUFFER_API_SLOT_COUNT

#define W_TRANSFOMRM_IDX 0
#define VP_TRANSFORM_IDX 1

#define INVALID_SLOT_IDX -1
#define INVALID_BUFFER_SIZE 0
#define CONSTANT_BUFFER_NAME_LEN_MAX 59

#include "SSEngineDefault/SSNativeTypes.h"

// Shader Constant Buffer Element's reflection data
struct SSShaderCBReflectionElement { 

	uint16 CBSize = INVALID_BUFFER_SIZE; // 2byte
	int8 CBSlotIdx = INVALID_SLOT_IDX; // 1byte

	uint8 bCBUsedByVSShader = false; // 1byte
	uint8 bCBUsedByPSShader = false; // 1byte

	char CBName[CONSTANT_BUFFER_NAME_LEN_MAX] = {}; // 59byte

}; // entirely 48byte

typedef struct _SSShaderReflectionForMaterial {

	SSShaderCBReflectionElement VSCBReflectionInfo[CONSTANT_BUFFER_COUNT_MAX];
	uint8 VSConstBufferNum = 0;

	SSShaderCBReflectionElement PSCBReflectionInfo[CONSTANT_BUFFER_COUNT_MAX];
	uint8 PSConstBufferNum = 0;

	SSShaderCBReflectionElement EntireCBReflectionInfo[CONSTANT_BUFFER_COUNT_MAX];
	uint8 EntireConstBufferNum = 0;
	uint8 ConstBufferSlotMax = 0;

	uint8 _texturePoolCount = 0;
	uint8 SamplerCount = 0;

} SSShaderReflectionForMaterial;