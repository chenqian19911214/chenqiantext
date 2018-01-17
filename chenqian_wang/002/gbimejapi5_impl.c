#include "gbimejapi5_impl.h"
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <assert.h>
//#include <fcntl.h>
//#include <sys/stat.h>

#include "gbapi.h"

///////////////////////////////////////////////////////////////////////////////////////////////////////
//
///////////////////////////////////////////////////////////////////////////////////////////////////////

static unsigned char _is_null_utf8_str( const char* str)
{
	return (str==NULL||strlen(str)<=0);
}

static unsigned char _is_null_wstr( GBLPCWCHAR wstr)
{
	return (wstr==NULL|| wstr[0]==0x0000);
}

/*
static int _ucs2uft8(const unsigned short* pText,char* pOut)
{
	const char* pchar = (const char*)pText;
  	unsigned short u =pText[0];

	if(pText[0] >= 0x0000 && pText[0] <= 0x007F)
	{
		pOut[0] = (char)pText[0];
	        return 1;
	}
	if(pText[0] >= 0x0080 && pText[0] <= 0x07FF)
	{
		pOut[0] = (0xE0 | ((pchar[1] & 0xF0) >> 4));
		pOut[1] = (0x80 | ((pchar[1] & 0x0F) << 2)) + ((pchar[0] & 0xC0) >> 6);
		return 2;
	}
	if(pText[0] >= 0x0800 && pText[0] <= 0xFFFF)
	{
		pOut[0] = (0xE0 | ((pchar[1] & 0xF0) >> 4));
		pOut[1] = (0x80 | ((pchar[1] & 0x0F) << 2)) + ((pchar[0] & 0xC0) >> 6);
		pOut[2] = (0x80 | (pchar[0] & 0x3F));
	}
	return 3;
}
*/

///////////////////////////////////////////////////////////////////////////////////////////////////////
//
///////////////////////////////////////////////////////////////////////////////////////////////////////



///////////////////////////////////////////////////////////////////////////////////////////////////////
//
///////////////////////////////////////////////////////////////////////////////////////////////////////


enum{ GBIME_MAX_LANG = 2};
struct _GBIMEV5CORE
{
	// 目前最多只支持两种语言切换
	GBLDB      m_ldbs[GBIME_MAX_LANG];
	// 输入法状态保存以及数据交换区域
	GBInputStruct  m_gbis;
	// 辅助输出结构体
	GBAuxInfo  m_aux;
	//
	GBBOOL	m_bEngineOpened;
	//	
	GBBOOL	m_bInputModeIsValid;
	//
	GBBOOL	m_bMasterLDBOpened;
	//	
	GBBOOL	m_bMasterUDBOpened;
	//
	GBBOOL	m_bSlaveLDBOpened;
	//	
	GBBOOL	m_bSlaveUDBOpened;
	//
	GBI32	m_gbi32MasterUDBId;
        //
	GBI32	m_gbi32SlaveUDBId;
	//
	GBINT m_nUDBUpdateFreq;
	//
	fpGBUDBUpdate_t m_fpUDBUpdateFunction;
	//
	GBU32 m_nUDBEnumIndex;
	//
	fpGetWStringWidth_t m_fpGetWStringWidth;

	//
	CALLBACKPARAM m_pCallbackParam;

	//
	unsigned short* m_pRegSymbolsBuffer;

	// 指示是否处于联想状态，若为真，使用API翻页，否则使用MMI翻页，默认为FALSE
	GBBOOL m_IsAssocState;

	GBWCHAR  mPinyinCode[64];
};

///////////////////////////////////////////////////////////////////////////////////////////////////////
//
///////////////////////////////////////////////////////////////////////////////////////////////////////


static void _coreInit(GBIMEV5CORE core)
{
	memset( &(core->m_ldbs), 0x0,sizeof(core->m_ldbs) );
	memset( &(core->m_gbis), 0x0, sizeof(core->m_gbis) );
	memset( &(core->m_aux), 0x0, sizeof(core->m_aux) );
	core->m_bEngineOpened = GBFalse;
	core->m_bInputModeIsValid = GBFalse;
	core->m_bMasterLDBOpened=GBFalse;
	core->m_bSlaveLDBOpened=GBFalse;
	core->m_bMasterUDBOpened = GBFalse;
	core->m_bSlaveUDBOpened = GBFalse;
	core->m_gbi32MasterUDBId = -1;
	core->m_gbi32SlaveUDBId = -1;
	core->m_nUDBUpdateFreq = GB_UDB_UPDATE_FREQ;
	core->m_fpUDBUpdateFunction = NULL;
	core->m_nUDBEnumIndex = 0;
	core->m_fpGetWStringWidth = NULL;
	core->m_pRegSymbolsBuffer = NULL;
	core->m_IsAssocState = GBFalse;
	memset( core->mPinyinCode, 0x0, sizeof(core->mPinyinCode) );
}



static GBRETURN _openLDB(GBIMEV5CORE core,GBLPCVOID master_ldb, GBLPCVOID slave_ldb)
{
	GBRETURN ret = GB_Failed;
	GBU32 data_size = 0x0;
	GBU8  lang_count = 1;
	GBGlobalPageOption gpo;

	/* 必须指定主语言 */
	if( master_ldb == NULL )
		return GB_Failed;
		

	core->m_ldbs[0].pData = master_ldb;
	if(  (ret=GBGetDataLanguage( 	core->m_ldbs[0].pData, 
				     	&(core->m_ldbs[0].nLang), 
					&(core->m_ldbs[0].nSubLang), 
					&data_size )) != GBOK )
		return ret;
	

	
	if( slave_ldb != NULL )
	{
		core->m_ldbs[1].pData  = slave_ldb;
		if(  (ret=GBGetDataLanguage( 	core->m_ldbs[1].pData, 
			        		&(core->m_ldbs[1].nLang), 
						&(core->m_ldbs[1].nSubLang), 
						&data_size )) != GBOK )
			return ret;
		lang_count++;
	}

	/* 总初始化*/
	core->m_gbis.pAuxInfo = &(core->m_aux);
	core->m_gbis.engine_private_buffer_size = sizeof(core->m_gbis.engine_private_buffer);
	if( (ret=GBInitialize( 	&(core->m_gbis),
				core->m_ldbs,
				lang_count )) != GBOK )
		return ret;

	if(lang_count>0)
		core->m_bMasterLDBOpened=GBTrue;
	if(lang_count>1)		
		core->m_bSlaveLDBOpened=GBTrue;

	/* 设置主语言为当前语言 */
	if( (ret=GBSetCurLanguage( 	&(core->m_gbis),
					core->m_ldbs[0].nLang,
					core->m_ldbs[0].nSubLang )) != GBOK )
		return ret;

	/* 设置候选类型和行数*/
	if( (ret=GBMMISetDefaultCandListType( &(core->m_gbis),GBCL_ROW_KEYBD)) != GBOK )
		return ret;
	if( (ret=GBMMISetCandRowCount( &(core->m_gbis), GBCS_INPUT, 1)) != GBOK )
		return ret;
	if( (ret=GBMMISetCandRowCount( &(core->m_gbis), GBCS_ASSOCIATE, 1)) != GBOK )
		return ret;

	/*设置分页参数*/
	memset((void*)&gpo,0x00,sizeof(GBGlobalPageOption));
	gpo.fpGetWStringWidth    = NULL;
	gpo.pGetStringWidthParam = 0;
	gpo.mPageOption.nMaxCandCountPerPage = \
	gpo.mChnCandPageOption.nMaxCandCountPerPage = \
	gpo.mSyllablePageOption.nMaxCandCountPerPage = 1;
	if( (ret=GBSetPageOption(&(core->m_gbis),&gpo)) != GBOK )
		return ret;


	if((ret = GBSetEngineOption(	&(core->m_gbis), 
		                    	GB_ENGINE_FUNCTION, 
					GBMMI_FUNCTION_OPTION_DEFAULT 
					| GBFO_Chn_Switch_Alp  
		/* | GBFO_Alp_UAW_Confirm è?1??ò?a?a??????￡?3???"Add?"oó￡?°?cancelòy??2??y3￡*/
					| GBFO_Multi_Tap_Show_Cand_Wnd /*2??a?a??￡?×???oò??2??á3???*/
					, 
					1 ))!=GBOK)
		return ret;

	
	core->m_bEngineOpened = GBTrue;
	return GBOK;
}

static GBRETURN _openMasterUDB(GBIMEV5CORE core,GBLPVOID master_udb, GBINT master_udb_size )
{
	GBRETURN ret = GB_Failed;
	
	if( master_udb==NULL)
		return GB_Failed;
	if( master_udb_size<=0)
		return GB_Failed;
	if( core->m_bMasterLDBOpened != GBTrue)
		return GB_Failed;

	if( (ret=GBUDBAttach( 	&(core->m_gbis), 
				master_udb, 
				master_udb_size,
				core->m_ldbs[0].nLang,
				core->m_ldbs[0].nSubLang, 
				&(core->m_gbi32MasterUDBId) )) != GBOK )
		return ret;
	
	core->m_bMasterUDBOpened = GBTrue;
	return GBOK;
}

static GBRETURN _openSlaveUDB(GBIMEV5CORE core,GBLPVOID slave_udb, GBINT slave_udb_size )
{
	GBRETURN ret = GB_Failed;
	
	if( slave_udb==NULL)
		return GB_Failed;
	if( slave_udb_size<=0)
		return GB_Failed;
	if( core->m_bSlaveLDBOpened != GBTrue)
		return GB_Failed;

	if( (ret=GBUDBAttach( 	&(core->m_gbis), 
				slave_udb, 
				slave_udb_size,
				core->m_ldbs[1].nLang,
				core->m_ldbs[1].nSubLang, 
				&(core->m_gbi32SlaveUDBId) )) != GBOK )
		return ret;
	
	core->m_bSlaveUDBOpened = GBTrue;
	return GBOK;
}

static void _resetStateWhileNoInput(GBIMEV5CORE core)
{
	/* 不断按CANCEL取消无缝切换之后，就会出现这种FUCKING状态，
	再按CANCEL也没用，必须RESET*/
	if( gbimejapi5_impl_get_cur_state(core) == GBIMS_INPUT && core->m_gbis.inputStringLen==0 )
		GBMMIReset(&(core->m_gbis));
}

static GBBOOL _isAlpNoMoreCandState(GBIMEV5CORE core)
{
	return ( gbimejapi5_impl_get_cur_state(core) == GBIMS_ALP_UAW_NO_MORE_CAND );
}

///////////////////////////////////////////////////////////////////////////////////////////////////////
//
///////////////////////////////////////////////////////////////////////////////////////////////////////


GBIMEV5CORE gbimejapi5_impl_new_instance(
#ifndef WIN32
	JNIEnv *env, jclass cls
#endif
	)
{
	GBIMEV5CORE core = (GBIMEV5CORE)malloc(sizeof(struct _GBIMEV5CORE));
	if(core==NULL)
		return NULL;

	core->m_pCallbackParam = (CALLBACKPARAM)malloc(sizeof(struct _CALLBACKPARAM));
#ifndef WIN32
	core->m_pCallbackParam->_env=env;
	core->m_pCallbackParam->_cls=cls;
#endif
	core->m_pCallbackParam->_core=core;

	_coreInit(core);
	return core;
}

void gbimejapi5_impl_delete_instance(GBIMEV5CORE core)
{
	gbimejapi5_impl_close(core);
	free((void*)core);
	free((void*)core->m_pCallbackParam);
}

void gbimejapi5_impl_close(GBIMEV5CORE core)
{
	if( core->m_bMasterUDBOpened )
		GBUDBDetach( &(core->m_gbis), core->m_gbi32MasterUDBId );
	
	if( core->m_bSlaveUDBOpened )
		GBUDBDetach( &(core->m_gbis), core->m_gbi32SlaveUDBId );

	if( core->m_pRegSymbolsBuffer != NULL)
		free(core->m_pRegSymbolsBuffer);

	GBTerminate(&(core->m_gbis));

	_coreInit(core);
}

GBBOOL gbimejapi5_impl_is_opened(GBIMEV5CORE core)
{
	return (GBIsInited(&(core->m_gbis))==GBOK && core->m_bEngineOpened);
}

int gbimejapi5_impl_getFileSize(GBACHAR *path)
{
	int size;
	FILE* fp = fopen( path, "r" );
	if (fp==NULL) return -2;
	fseek( fp, 0L, SEEK_END );
	size = ftell(fp);
	fclose(fp);
	return size;

}
GBLPCVOID gbimejapi5_impl_LoadEBDData(GBACHAR *path)
{
	FILE * fp = NULL;

	int size;
	GBLPCVOID data;

	size = gbimejapi5_impl_getFileSize(path);

	fp = fopen(path, "rb");

	//printf("%d", size);

	data = (GBLPCVOID)malloc(size);
	if(fread((GBU8*)data, 1, size, fp) != size)
	{
		printf("read error");
	}


	return data;
}

GBRETURN gbimejapi5_impl_open(GBIMEV5CORE core,GBLPCVOID master_ldb)
{
	GBRETURN ret = GB_Failed;
	gbimejapi5_impl_close(core);

	while(1)
	{
		if((ret=_openLDB(core,master_ldb,NULL))!=GBOK)
			break;

		return GBOK;
	}
	
	gbimejapi5_impl_close(core);
	return ret;
}

GBRETURN gbimejapi5_impl_open_ex(GBIMEV5CORE core,GBLPCVOID master_ldb,GBLPCVOID slave_ldb)
{
	GBRETURN ret = GB_Failed;
	gbimejapi5_impl_close(core);

	while(1)
	{
		if((ret=_openLDB(core,master_ldb,slave_ldb))!=GBOK)
			break;

		return GBOK;
	}
	
	gbimejapi5_impl_close(core);
	return ret;
}

GBRETURN gbimejapi5_impl_open_with_udb(GBIMEV5CORE core,GBLPCVOID master_ldb,GBLPVOID master_udb,GBINT master_udb_size)
{
	GBRETURN ret = GB_Failed;
	gbimejapi5_impl_close(core);

	while(1)
	{
		if((ret=_openLDB(core,master_ldb,NULL))!=GBOK)
			break;
		if((ret=_openMasterUDB(core,master_udb,master_udb_size))!=GBOK)
			break;
		return GBOK;
	}

	gbimejapi5_impl_close(core);
	return ret;
}


GBRETURN gbimejapi5_impl_open_ex_with_udb(
GBIMEV5CORE core,
GBLPCVOID master_ldb,
GBLPVOID master_udb,
GBINT master_udb_size,
GBLPCVOID slave_ldb,
GBLPVOID slave_udb,
GBINT slave_udb_size)
{
	GBRETURN ret = GB_Failed;
	gbimejapi5_impl_close(core);

	while(1)
	{
		if((ret=_openLDB(core,master_ldb,slave_ldb))!=GBOK)
			break;
		if((ret=_openMasterUDB(core,master_udb,master_udb_size))!=GBOK)
			break;
		if((ret=_openSlaveUDB(core,slave_udb,slave_udb_size))!=GBOK)
			break;
		return GBOK;
	}

	gbimejapi5_impl_close(core);
	return ret;
}


void gbimejapi5_impl_enable_udb_update_callback(GBIMEV5CORE core,fpGBUDBUpdate_t fpUpdateFunction)
{
	core->m_fpUDBUpdateFunction = fpUpdateFunction;
	
	if(core->m_bMasterUDBOpened )
	{
		GBUDBSetUpdateParam(	&(core->m_gbis), 
					core->m_gbi32MasterUDBId,
					core->m_nUDBUpdateFreq,
					core->m_fpUDBUpdateFunction,
					(GBLPVOID)core->m_pCallbackParam );
	}
	
	if(core->m_bSlaveUDBOpened )
	{
		GBUDBSetUpdateParam(	&(core->m_gbis), 
					core->m_gbi32SlaveUDBId,
					core->m_nUDBUpdateFreq,
					core->m_fpUDBUpdateFunction,
					(GBLPVOID)core->m_pCallbackParam );
	}
}

GBRETURN gbimejapi5_impl_set_udb_update_freq(GBIMEV5CORE core,GBINT freq)
{
	GBRETURN ret = GB_Failed;
	core->m_nUDBUpdateFreq = freq;
	
	while(1)
	{

		if(core->m_bMasterUDBOpened )
		{
			if((ret=GBUDBSetUpdateParam(	&(core->m_gbis), 
							core->m_gbi32MasterUDBId,
							core->m_nUDBUpdateFreq,
							core->m_fpUDBUpdateFunction,
							(GBLPVOID)core->m_pCallbackParam ))!=GBOK)
				break;
		}
	
		if(core->m_bSlaveUDBOpened )
		{
			if((ret=GBUDBSetUpdateParam(	&(core->m_gbis), 
							core->m_gbi32SlaveUDBId,
							core->m_nUDBUpdateFreq,
							core->m_fpUDBUpdateFunction,
							(GBLPVOID)core->m_pCallbackParam ))!=GBOK)
				break;
		}
		
		return GBOK;
	}

	gbimejapi5_impl_set_udb_update_freq(core,GB_UDB_UPDATE_FREQ);
	return ret;	
}

void gbimejapi5_impl_clear_udb_data(GBIMEV5CORE core)
{
	GBUDBDataReset(&(core->m_gbis));
}

GBRETURN gbimejapi5_impl_delete_udb_phrase(GBIMEV5CORE core,GBLPCWCHAR phrase)
{
	return GBUDBDeletePhrase(&(core->m_gbis),phrase);
}

GBRETURN gbimejapi5_impl_add_udb_phrase(GBIMEV5CORE core,GBLPCWCHAR phrase,GBLPCWCHAR syll)
{
	return GBUDBAddPhrase(&(core->m_gbis),phrase,syll);
}


void gbimejapi5_impl_start_enum_udb_phrase(GBIMEV5CORE core)
{
	core->m_nUDBEnumIndex = 0;
}

GBRETURN gbimejapi5_impl_get_next_udb_phrase(GBIMEV5CORE core,GBLPWCHAR pBuffer,GBINT nBuffLen)
{
	return GBUDBGetNextPhrase(&(core->m_gbis),&(core->m_nUDBEnumIndex),pBuffer,nBuffLen);
}

void gbimejapi5_impl_enable_paging_callback(GBIMEV5CORE core,fpGetWStringWidth_t fpGetWStringWidth)
{
	GBGlobalPageOption pageOptions;
	if(gbimejapi5_impl_is_opened(core)!=GBTrue)
		return;

	core->m_fpGetWStringWidth = fpGetWStringWidth;
	
	if( GBGetPageOption( &(core->m_gbis), &pageOptions) != GBOK )
		return;
	
	pageOptions.fpGetWStringWidth    = core->m_fpGetWStringWidth;
	pageOptions.pGetStringWidthParam = (void*)core->m_pCallbackParam;

	GBSetPageOption( &(core->m_gbis), &pageOptions);
}

GBRETURN gbimejapi5_impl_set_alp_paging_options(GBIMEV5CORE core,GBINT page_width,GBINT word_space,GBINT max_cand_per_page)
{
	GBGlobalPageOption pageOptions;
	GBRETURN ret = GB_Failed;

	if( (ret=GBGetPageOption( &(core->m_gbis), &pageOptions)) != GBOK )
		return ret;

	pageOptions.mPageOption.nTotalWidthInPixel = page_width;
	pageOptions.mPageOption.nSplitWidthInPixel = word_space;
	pageOptions.mPageOption.nMaxCandCountPerPage = max_cand_per_page;

	if( (ret=GBSetPageOption( &(core->m_gbis), &pageOptions)) != GBOK )
		return ret;

	return GBOK;
}

GBRETURN gbimejapi5_impl_set_chn_paging_options(GBIMEV5CORE core,GBINT page_width,GBINT word_space,GBINT max_cand_per_page)
{
	GBGlobalPageOption pageOptions;
	GBRETURN ret = GB_Failed;

	if( (ret=GBGetPageOption( &(core->m_gbis), &pageOptions)) != GBOK )
		return ret;

	pageOptions.mChnCandPageOption.nTotalWidthInPixel = page_width;
	pageOptions.mChnCandPageOption.nSplitWidthInPixel = word_space;
	pageOptions.mChnCandPageOption.nMaxCandCountPerPage = max_cand_per_page;

	if( (ret=GBSetPageOption( &(core->m_gbis), &pageOptions)) != GBOK )
		return ret;

	return GBOK;
}

GBRETURN gbimejapi5_impl_set_syll_paging_options(GBIMEV5CORE core,GBINT page_width,GBINT word_space,GBINT max_cand_per_page)
{
	GBGlobalPageOption pageOptions;
	GBRETURN ret = GB_Failed;

	if( (ret=GBGetPageOption( &(core->m_gbis), &pageOptions)) != GBOK )
		return ret;

	pageOptions.mSyllablePageOption.nTotalWidthInPixel = page_width;
	pageOptions.mSyllablePageOption.nSplitWidthInPixel = word_space;
	pageOptions.mSyllablePageOption.nMaxCandCountPerPage = max_cand_per_page;

	if( (ret=GBSetPageOption( &(core->m_gbis), &pageOptions)) != GBOK )
		return ret;

	return GBOK;
}

GBRETURN gbimejapi5_impl_set_input_mode(GBIMEV5CORE core, GBINT input_mode,GBINT cand_type)
{
	GBRETURN ret = GB_Failed;
	while(1)
	{
		if((ret=GBSetInputMode( &(core->m_gbis), input_mode )) != GBOK )
			break;
		if( cand_type != GBCT_Normal )
		{
			if( (ret=GBSetCandType( &(core->m_gbis),cand_type)) != GBOK )
				break;
		}
		
		return GBOK;
	}

	return ret; 
}

GBINT gbimejapi5_impl_get_cur_state(GBIMEV5CORE core)
{
	return core->m_gbis.pAuxInfo->nStatus;
}

static GBRETURN _inputKeyEvt(GBIMEV5CORE core, GBINT evt)
{
	core->m_IsAssocState = GBFalse;
	return GBMMIHandleKeyEvent(&(core->m_gbis),evt,0x0);
}

GBRETURN gbimejapi5_impl_input_event(GBIMEV5CORE core, GBINT evt)
{
	GBRETURN ret = GB_Failed;

	const GBINT cur_state = gbimejapi5_impl_get_cur_state(core);

	core->m_IsAssocState = GBFalse;

	/* 以下状态可以直接输入*/
	if(	cur_state == GBIMS_INITIAL ||
		cur_state == GBIMS_INPUT   ||	
		cur_state == GBIMS_CHN_UAW ||
		cur_state == GBIMS_ALP_UAW_NO_MORE_CAND ||
		cur_state == GBIMS_ALP_UAW ||
		cur_state == GBIMS_ALP_UAW_BEGIN ||
		cur_state == GBIMS_ALP_UAW_Confirm ||
		cur_state == GBIMS_ASSOCIATE ||
		cur_state == GBIMS_MULTITAP_INTERPUNCTION )
	{
		//OK
	}
	/* 以下状态需要回滚 // 发BACK，不是发UP !!*/
	else if( cur_state == GBIMS_SELECTED )
	{
		if((ret=_inputKeyEvt(core,GBKEY_BACK))!=GBOK)
			return ret; 
	}
	else
		return GB_Failed;
	
	switch( evt )
	{
		case GBKEY_BACK:

		case GBKEY_0:
		case GBKEY_1:
		case GBKEY_2:
		case GBKEY_3:
		case GBKEY_4:
		case GBKEY_5:
		case GBKEY_6:
		case GBKEY_7:
		case GBKEY_8:
		case GBKEY_9:

		case GBKEY_A:
		case GBKEY_B:
		case GBKEY_C:
		case GBKEY_D:
		case GBKEY_E:
		case GBKEY_F:
		case GBKEY_G:
		case GBKEY_H:
		case GBKEY_I:
		case GBKEY_J:
		case GBKEY_K:
		case GBKEY_L:
		case GBKEY_M:
		case GBKEY_N:
		case GBKEY_O:
		case GBKEY_P:
		case GBKEY_Q:
		case GBKEY_R:
		case GBKEY_S:
		case GBKEY_T:
		case GBKEY_U:
		case GBKEY_V:
		case GBKEY_W:
		case GBKEY_X:
		case GBKEY_Y:
		case GBKEY_Z:
                ret = _inputKeyEvt(core,evt);
                break;

	default:
		//assert(0);
		return GB_Failed;
	} // end switch( evt )

	_resetStateWhileNoInput(core);
	return ret;
}

GBRETURN gbimejapi5_impl_input_event_ex(GBIMEV5CORE core, GBINT evt,GBU32 param)
{
	 core->m_IsAssocState = GBFalse;
	 return GBMMIHandleKeyEvent(&(core->m_gbis),evt,param);
}

GBRETURN gbimejapi5_impl_input_default_interpuction(GBIMEV5CORE core)
{
	GBRETURN ret = GBOK;
	GBINT cur_state = gbimejapi5_impl_get_cur_state(core);

	core->m_IsAssocState = GBFalse;

	/* 以下状态需要回滚 // 发BACK，不是发UP !!*/
	if( cur_state == GBIMS_SELECTED )
	{
		if((ret=_inputKeyEvt(core,GBKEY_BACK))!=GBOK)
			return ret; 
	}

	cur_state = gbimejapi5_impl_get_cur_state(core);
	
	if(	cur_state == GBIMS_INITIAL || 
		cur_state == GBIMS_INPUT   ||
		cur_state == GBIMS_ASSOCIATE  )
	{
		
		GBINT in_key = GBKEY_AMB_SPECIAL_A;

		if((ret = GBSetCandType( &(core->m_gbis),GBCT_Symbol))!=GBOK )
			return ret;

		if((ret=GBMMIRegisterInterpunctionKey(	&(core->m_gbis),
						  	in_key,
						  	NULL,
						  	6 ))!=GBOK)
			return ret;
		
		if((ret=GBIS_SetInputStringUpdated(&(core->m_gbis)))!=GBOK)
		{
			//return ret;
		}

		if((ret=_inputKeyEvt(core,in_key))!=GBOK)
		{
			//return ret;
		}

		if((ret=GBIS_SetInputStringUpdated(&(core->m_gbis)))!=GBOK)
		{
			//return ret;
		}
	}

	return GBOK;
}

void gbimejapi5_impl_clear_all_input(GBIMEV5CORE core)
{
	core->m_IsAssocState = GBFalse;
	GBMMIReset(&(core->m_gbis));
}

GBBOOL gbimejapi5_impl_has_candidate(GBIMEV5CORE core)
{
	return ( core->m_gbis.outputInfo.nCandNum > 0 );
}

GBINT gbimejapi5_impl_get_cand_count_in_page(GBIMEV5CORE core)
{
	return core->m_gbis.outputInfo.nCandNum;
}

GBINT gbimejapi5_impl_has_prev_cand_page(GBIMEV5CORE core)
{
	if( _isAlpNoMoreCandState(core) )
		return 1;
	else if( GBHavePrevPage( &(core->m_gbis) ) == GBOK )
		return 2;
	else
		return 0;
}

static GBBOOL _canEnterAlpSpellPage(GBIMEV5CORE core)
{
	const GBINT cur_state = gbimejapi5_impl_get_cur_state(core);
	if (	LANG_USE_ALP_ENGINE(core->m_gbis.nLang) &&
		( core->m_gbis.nInputMode == GBIM_Predictive) &&
		( core->m_gbis.inputStringLen > 1) &&
		( cur_state==GBIMS_INPUT || cur_state==GBIMS_SELECTED ) )
		return GBTrue;
	else
		return GBFalse;
}

GBINT gbimejapi5_impl_has_next_cand_page(GBIMEV5CORE core)
{
	if( GBHaveNextPage( &(core->m_gbis) ) == GBOK )
		return 2;
	else if( _canEnterAlpSpellPage(core)  )
		return 1;
	else
		return 0;
}

static GBBOOL _tryToEnterCandSelectState(GBIMEV5CORE core)
{
	if( gbimejapi5_impl_has_candidate(core) )
	{
		const GBINT cur_state = gbimejapi5_impl_get_cur_state(core);
		
		if( cur_state == GBIMS_SELECTED )
			return GBTrue;

		if(	cur_state == GBIMS_INPUT     || 
			cur_state == GBIMS_CHN_UAW   ||
			cur_state == GBIMS_ALP_UAW   || 
			cur_state == GBIMS_ASSOCIATE ||
			cur_state == GBIMS_MULTITAP_INTERPUNCTION )
		{
			_inputKeyEvt(core,GBKEY_DOWN);
			assert( gbimejapi5_impl_get_cur_state(core) == GBIMS_SELECTED );
			return GBTrue;
		}
	}
	return GBFalse;
}

GBBOOL gbimejapi5_impl_move_prev_cand_page(GBIMEV5CORE core)
{
	const GBINT ret = gbimejapi5_impl_has_prev_cand_page(core);
	if( ret != 0 )
	{
		if( ret == 1 )
		{
			_inputKeyEvt(core,GBKEY_UP);
			return GBTrue;
		}
		else if( core->m_IsAssocState == GBTrue)
		{
			GBPrevPageCandidate(&core->m_gbis, core->m_gbis.outputInfo.nFirstCandIndex - 1);
			return GBTrue;
		}
		else if( _tryToEnterCandSelectState(core) )
 		{
 			_inputKeyEvt(core,GBKEY_UP);
 			return GBTrue;
 		}
	}
	return GBFalse;
}

GBBOOL gbimejapi5_impl_move_next_cand_page(GBIMEV5CORE core)
{
	const GBINT ret = gbimejapi5_impl_has_next_cand_page(core);	
	if( ret != 0 )
	{
		if( ret == 1 )
		{
			_inputKeyEvt(core,GBKEY_DOWN);
			return GBTrue;
		}
		else if( core->m_IsAssocState == GBTrue)
		{
			GBNextPageCandidate(&core->m_gbis, core->m_gbis.outputInfo.nFirstCandIndex + core->m_gbis.outputInfo.nCandNum);
			return GBTrue;
		}
		else if( _tryToEnterCandSelectState(core) )
		{
			_inputKeyEvt(core,GBKEY_DOWN);
			return GBTrue;
		}

	}
	return GBFalse;
}

GBLPCWCHAR gbimejapi5_impl_get_cand_in_page_by_idx(GBIMEV5CORE core,GBINT idx)
{
	if( idx < 0 || idx >= gbimejapi5_impl_get_cand_count_in_page(core) )
		return NULL;
	return core->m_gbis.outputInfo.pCandidates[idx];
}

void gbimejapi5_impl_select_cand_in_page_by_idx(GBIMEV5CORE core,GBINT idx)
{
	const GBINT cur_state = gbimejapi5_impl_get_cur_state(core);
	if( idx < 0 || idx >= gbimejapi5_impl_get_cand_count_in_page(core) )
		return;


	if( cur_state == GBIMS_ALP_UAW_NO_MORE_CAND ||
	    cur_state == GBIMS_ALP_UAW_BEGIN        ||
	    cur_state == GBIMS_ALP_UAW_Confirm         )
	{
		_inputKeyEvt(core,GBKEY_OK);
	}
	else if( _tryToEnterCandSelectState(core) )
	{
		GBINT move_step=0;
				
		if( idx < core->m_gbis.pAuxInfo->nSelCol )
		{
			for(move_step=core->m_gbis.pAuxInfo->nSelCol; move_step>idx;--move_step)
				_inputKeyEvt(core,GBKEY_PREV_CANDIDATE);
		}
		else if( idx > core->m_gbis.pAuxInfo->nSelCol )
		{
			for(move_step=core->m_gbis.pAuxInfo->nSelCol;move_step<idx;++move_step)
				_inputKeyEvt(core,GBKEY_NEXT_CANDIDATE);
		}

		_inputKeyEvt(core,GBKEY_OK);

	}
}

GBBOOL gbimejapi5_impl_has_syllable(GBIMEV5CORE core)
{
	return ( INPUT_MODE_IS_AMB_CHN(core->m_gbis.nInputMode) &&
		 GBIS_HaveSyllableSelection(&(core->m_gbis)) );
}

GBINT gbimejapi5_impl_get_syll_count_in_page(GBIMEV5CORE core)
{
	return core->m_gbis.pAuxInfo->nSyllableNum;
}

GBBOOL gbimejapi5_impl_has_prev_syll_page(GBIMEV5CORE core)
{
	return ( core->m_gbis.pAuxInfo->nSyllablePageStartIndex > 0 );
}

GBBOOL gbimejapi5_impl_has_next_syll_page(GBIMEV5CORE core)
{
	return ( GBIS_HaveMoreSyllable(&(core->m_gbis)) );
}


static GBBOOL _tryToEnterSyllState(GBIMEV5CORE core)
{
	if( gbimejapi5_impl_has_syllable(core) )
	{
		GBINT cur_state = gbimejapi5_impl_get_cur_state(core);
		
		if( cur_state == GBIMS_INPUT ||	cur_state == GBIMS_CHN_UAW   )
			return GBTrue;
		
		if( cur_state == GBIMS_SELECTED )
		{
			_inputKeyEvt(core,GBKEY_BACK);
			cur_state = gbimejapi5_impl_get_cur_state(core);
			assert( cur_state == GBIMS_INPUT || cur_state == GBIMS_CHN_UAW );
			return GBTrue;
		}
	}
	return GBFalse;
}

GBBOOL gbimejapi5_impl_move_prev_syll_page(GBIMEV5CORE core)
{
	if( _tryToEnterSyllState(core) && gbimejapi5_impl_has_prev_syll_page(core) )
	{
		_inputKeyEvt(core,GBKEY_PREV_PAGE_SYLLABLE);
		return GBTrue;
	}
	return GBFalse;
}

GBBOOL gbimejapi5_impl_move_next_syll_page(GBIMEV5CORE core)
{
	if( _tryToEnterSyllState(core) && gbimejapi5_impl_has_next_syll_page(core) )
	{
		_inputKeyEvt(core,GBKEY_NEXT_PAGE_SYLLABLE);
		return GBTrue;
	}
	return GBFalse;
}

GBLPCWCHAR  gbimejapi5_impl_get_syll_in_page_by_idx(GBIMEV5CORE core,GBINT idx)
{
	if( idx< 0 || idx >= gbimejapi5_impl_get_syll_count_in_page(core) )
		return NULL;
	return core->m_gbis.pAuxInfo->pSyllables[idx];
}

void gbimejapi5_impl_select_syll_in_page_by_idx(GBIMEV5CORE core,GBINT idx)
{	
	if( idx< 0 || idx >= gbimejapi5_impl_get_syll_count_in_page(core) )
		return;
	if( _tryToEnterSyllState(core) )
		gbimejapi5_impl_input_event_ex(core,GBEVENT_SELECT_SYLLABLE_PAGED,idx);
}



GBINT gbimejapi5_impl_get_selected_syll_Idx(GBIMEV5CORE core)
{
	return core->m_gbis.pAuxInfo->nSyllableIndex;
}

GBLPCWCHAR  gbimejapi5_impl_get_input_string(GBIMEV5CORE core)
{
	if(_is_null_wstr(core->m_gbis.inputString))
		return NULL;
	return core->m_gbis.inputString;
}

GBLPCWCHAR  gbimejapi5_impl_get_output_string(GBIMEV5CORE core)
{
	if(_is_null_wstr(core->m_gbis.pAuxInfo->pOutputString))
		return NULL;
	return core->m_gbis.pAuxInfo->pOutputString;
}

GBLPCWCHAR  gbimejapi5_impl_get_upscreen_string(GBIMEV5CORE core)
{
	if(_is_null_wstr(core->m_gbis.outputInfo.pUpScreenStr))
		return NULL;
	return core->m_gbis.outputInfo.pUpScreenStr;
}

void gbimejapi5_impl_clear_upscreen_string(GBIMEV5CORE core)
{
	if( _is_null_wstr(core->m_gbis.outputInfo.pUpScreenStr) )
		return;
	core->m_gbis.outputInfo.pUpScreenStr[0]=0x0;
}

static void _updateComponents(GBIMEV5CORE core)
{
	GBChnGetComponents(&(core->m_gbis));
}

GBLPCWCHAR gbimejapi5_impl_get_stroke_string(GBIMEV5CORE core)
{
	_updateComponents(core);
	if( _is_null_wstr(core->m_gbis.pAuxInfo->wStroke) )
		return NULL;
	else
		return core->m_gbis.pAuxInfo->wStroke;
}

GBINT gbimejapi5_impl_get_components_count(GBIMEV5CORE core)
{
	GBPCWCHAR pp = NULL;
	_updateComponents(core);
	if( _is_null_wstr(core->m_gbis.pAuxInfo->wComponents) )
		return 0;
	pp = core->m_gbis.pAuxInfo->wComponents;
	while(*pp!=0x0000)pp++;
	return pp - core->m_gbis.pAuxInfo->wComponents;
}

/*
-(NSString*)getComponentStringByIdx:(int)idx
{
	if( idx<0 || idx>=[self getComponentsCount] )
		return Nil;
	GBWCHAR wstr[2]={0x0000,0x0000};
	wstr[0]=m_gbis.pAuxInfo->wComponents[idx];
	return ucs_to_nsstring(wstr);
}
*/

unsigned short gbimejapi5_impl_get_component_unicode_by_idx(GBIMEV5CORE core,GBINT idx)
{
	if( idx<0 || idx>=gbimejapi5_impl_get_components_count(core) )
		return 0x0000;
	return core->m_gbis.pAuxInfo->wComponents[idx];
}

GBRETURN gbimejapi5_impl_set_assocword(GBIMEV5CORE core, GBLPCWCHAR strConfirmedWord,GBBOOL reset)
{
	GBRETURN ret = GB_Failed;	

	while(1)
	{

		if( _is_null_wstr(strConfirmedWord) )
			break;

	
	//GBMMIReset(&m_gbis);
	
		if( (ret=GBSetCandType( &(core->m_gbis),GBCT_Associate)) != GBOK )
			break;

// 	if( (ret=GBSetAssocWord( &(core->m_gbis), strConfirmedWord, reset )) != GBOK )
// 		return ret;
// 
// 	GBIS_SetInputStringUpdated(&(core->m_gbis));
// 	
// 	// í?1y?a??FUCKING·?·??üD?μ±?°oò??
// 	if( (ret=GBNextPageCandidate( &(core->m_gbis), 
// 	 /*m_gbis.outputInfo.nCandNum + m_gbis.outputInfo.nFirstCandIndex*/0)) != GBOK)
// 		return ret;
	
		if( (ret=GBGetAssocWord( &(core->m_gbis), strConfirmedWord, reset )) != GBOK )
			break;

		core->m_IsAssocState=GBTrue;
		return GBOK;
	}

	core->m_IsAssocState=GBFalse;
	return ret;
}

GBRETURN gbimejapi5_impl_set_capstate(
GBIMEV5CORE core,
GBINT bShiftDown,
GBINT bCapsLockToggle,
GBBOOL bUpdateCandidates)
{
	GBRETURN ret = GB_Failed;

	if( (ret=GBSetShiftCap( &(core->m_gbis), bShiftDown, bCapsLockToggle )) != GBOK )
		return ret;

	if( bUpdateCandidates )
	{

		_inputKeyEvt(core,GBKEY_1);
		_inputKeyEvt(core,GBKEY_BACK);
	}

	return GBOK;
}

GBRETURN gbimejapi5_impl_set_fuzzy_options(GBIMEV5CORE core, GBU32 config,GBINT on )
{
	return GBSetFuzzy(&(core->m_gbis),config,on);
}

GBRETURN gbimejapi5_impl_set_engine_option(GBIMEV5CORE core,GBINT engine_id,GBU32 config,GBU32 option)
{
	return GBSetEngineOption(&(core->m_gbis),engine_id,config,option);
}

GBRETURN gbimejapi5_impl_engine_switchto_slave_lang( GBIMEV5CORE core, GBINT input_mode )
{
	GBRETURN ret = GB_Failed;
	GBINT switch_key = GBKEY_AMB_SPECIAL_B;
	if( core->m_bSlaveLDBOpened != GBTrue )
		return GB_Failed;
	

	if((ret=GBMMISetSwitchOption( &(core->m_gbis),
		                      switch_key,
				      core->m_ldbs[1].nLang,
				      core->m_ldbs[1].nSubLang, 
				      input_mode )) != GBOK )
		return ret;

	_inputKeyEvt(core,switch_key);
	return GBOK;
}

void gbimejapi5_impl_enable_log(GBIMEV5CORE core,fpLogRecorder_t fpLogRecorder)
{
	GBAttachLogRecorder(&(core->m_gbis),fpLogRecorder,(GBLPVOID)core->m_pCallbackParam);
}

GBRETURN gbimejapi5_impl_registerInterpunctionKeyEx(
	GBIMEV5CORE core,
	GBINT cKey,
	GBLPWCHAR pSymbols,
	GBINT cSplit,
	GBINT nCandListType,
	GBINT nCandDesireRow,
	GBINT nCandCountPerRow,
	GBUINT nOption
)
{
	GBRETURN ret =  GBMMIRegisterInterpunctionKeyEx(
		&(core->m_gbis), 
		cKey,
		pSymbols,
		cSplit,
		nCandListType,
		nCandDesireRow,
		nCandCountPerRow,
		nOption
	);

	if( ret == GBOK )
	{
		if( core->m_pRegSymbolsBuffer != NULL)
			free(core->m_pRegSymbolsBuffer);
		core->m_pRegSymbolsBuffer=pSymbols;
	}
	return ret;
}

GBRETURN gbimejapi5_impl_select_cand_ex(GBIMEV5CORE core,GBLPCWCHAR cand,GBINT input_len)
{
	GBRETURN ret = GBSelectCandidateEx(&(core->m_gbis), cand, input_len );
	return GBMMIHandleKeyEvent(&(core->m_gbis),GBEVENT_UPDATE_CANDIDATE,0x0);
}

GBINT gbimejapi5_impl_get_cand_input_len_in_page_by_idx(GBIMEV5CORE core,GBINT idx)
{
	return core->m_gbis.pAuxInfo->nCandInputLen[idx];
}

GBRETURN gbimejapi5_impl_attach_mdb(GBIMEV5CORE core,GBLPCVOID mdb, GBINT mdb_len,GBINT lang,GBINT* pID)
{
	return  GBMDBAttach(&(core->m_gbis),mdb,lang,mdb_len,pID); 
}

GBRETURN gbimejapi5_impl_detach_mdb(GBIMEV5CORE core,GBINT ID)
{
	return GBMDBDetach(&(core->m_gbis),ID);
}

/**/
GBLPCWCHAR gbimejapi5_impl_get_pinyin_code(GBIMEV5CORE core,GBLPCWCHAR word,GBINT idx)
{
	GBRETURN ret = GBWord2CodesByInputmod(&(core->m_gbis), 
																			GBIM_Pinyin, 
																			word[0],
																			idx,
																			core->mPinyinCode);
	if(ret!=GBOK)
		memset(core->mPinyinCode,0x0,sizeof(core->mPinyinCode));
	return core->mPinyinCode;
}
GBRETURN gbimejapi5_impl_update_candidates(GBIMEV5CORE core)
{
	GBIS_SetInputStringUpdated(&core->m_gbis);
	return gbimejapi5_impl_input_event(core, GBEVENT_UPDATE_CANDIDATE);
}
///////////////////////////////////////////////////////////////////////////////////////////////////////
//
///////////////////////////////////////////////////////////////////////////////////////////////////////
#ifdef WIN32
GBIMEV5CORE g_core;
GBLPCVOID g_master_ldb;
int main()
{
	GBPCWCHAR pCode;
	g_core = gbimejapi5_impl_new_instance();
	g_master_ldb = gbimejapi5_impl_LoadEBDData("../../data/gb_sc_v5.ebd");

	gbimejapi5_impl_open(g_core, g_master_ldb);


	//gbimejapi5_impl_set_input_mode(g_core, GBIM_Qwerty_Pinyin, GBCT_Normal);

//	pCode = gbimejapi5_impl_get_pinyin_code(g_core, L"你", 0);

	gbimejapi5_impl_set_input_mode(g_core, GBIM_Pinyin, GBCT_Normal);

	
	_inputKeyEvt(g_core, GBKEY_9);
	//_inputKeyEvt(g_core, GBKEY_NEXT_PAGE_CANDIATE);
	_inputKeyEvt(g_core, GBKEY_OK);

	while(1)
		gbimejapi5_impl_move_next_cand_page(g_core);
//	gbimejapi5_impl_input_default_interpuction(g_core);

	gbimejapi5_impl_update_candidates(g_core);




	gbimejapi5_impl_delete_instance(g_core);
	free(g_master_ldb);
	return 0;
}
#endif
