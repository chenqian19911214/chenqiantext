#include "GBIMEEngineV52.h"
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include "gbapi.h"
#include "gbtype.h"
#include "gbwchar.h"
#include "gbimejapi5_impl.h"

/************************************** utilities  **************************************/

static unsigned short* _JString2UCS(JNIEnv *env, jstring jstr )
{
	if(jstr==NULL)
		return NULL;

	size_t str_len = (size_t)((*env)->GetStringLength(env,jstr));
	if(str_len<=0)
		return NULL;

	const unsigned int buf_size = str_len*2;
	const unsigned int tail_size =2;	
	unsigned char* ucs = (unsigned char*)malloc(buf_size+tail_size);
	if(ucs==NULL)
		return NULL;
	
	jchar* src = (*env)->GetStringChars(env,jstr,NULL);
	memcpy(ucs,(unsigned char*)src,buf_size);
	(*env)->ReleaseStringChars(env,jstr,src);

	unsigned int t=0;
	for(t=0;t<tail_size;++t)
		ucs[buf_size+t]=0x00;
	
	return (unsigned short*)ucs;	
}

static jstring _UCS2JString(JNIEnv *env, const unsigned short* ucs)
{
	if(ucs==NULL)
		return NULL;
	else
		return (*env)->NewString(env, (jchar*)ucs, (jsize)wcslen(ucs));
}

static jstring _UTF82JString(JNIEnv *env, const char* utf8)
{
	if(utf8==NULL)
		return NULL;
	else
		return (*env)->NewStringUTF(env,utf8);
}


/***************************** Callback defined in Java ******************************/

//static jmethodID s_jmd_onQueryStringWidth = NULL;
static jmethodID _onQueryStringWidth(JNIEnv *env, jclass cls)
{
	jmethodID s_jmd_onQueryStringWidth = NULL;
	if(s_jmd_onQueryStringWidth==NULL)
	{
		s_jmd_onQueryStringWidth = (*env)->GetStaticMethodID(
			env,
			cls,
			"onQueryStringWidth",
			"(JLjava/lang/String;)I"
		);	
	}
	return s_jmd_onQueryStringWidth;
}

static GBINT _fpGetWStringWidth(void* pParam, GBLPCWCHAR pWString)
{
	CALLBACKPARAM param = (CALLBACKPARAM)pParam;
	JNIEnv* env = param->_env;
	jclass  cls = param->_cls;
	return (*env)->CallStaticIntMethod(	env,
						cls,
						_onQueryStringWidth(env,cls),
						(jlong)param->_core,
						_UCS2JString(env,pWString)	);
}

//static jmethodID s_jmd_onUDBUpdated = NULL;
static jmethodID _onUDBUpdated(JNIEnv *env, jclass cls)
{
	jmethodID s_jmd_onUDBUpdated = NULL;
	if(s_jmd_onUDBUpdated==NULL)
	{
		s_jmd_onUDBUpdated = (*env)->GetStaticMethodID(
			env,
			cls,
			"onUDBUpdated",
			"(JI)V"
		);	
	}
	return s_jmd_onUDBUpdated;
}

static void _fpGBUDBUpdate(GBI32 nUDBID, GBLPVOID pData, GBU32 len, GBLPVOID pOEMUpdateParam)
{
	CALLBACKPARAM param = (CALLBACKPARAM)pOEMUpdateParam;
	JNIEnv* env = param->_env;
	jclass  cls = param->_cls;
	(*env)->CallStaticIntMethod(env,cls,_onUDBUpdated(env,cls),(jlong)param->_core,(jint)nUDBID);
}


static jmethodID _onLog(JNIEnv *env, jclass cls)
{
	jmethodID s_jmd_onLog = NULL;
	if(s_jmd_onLog==NULL)
	{
		s_jmd_onLog = (*env)->GetStaticMethodID(
			env,
			cls,
			"onLog",
			"(JLjava/lang/String;)V"
		);	
	}
	return s_jmd_onLog;
}


static GBINT _fpLogRecorder(
GBLPVOID pOEMData, 
const char * format,
unsigned long p1,
unsigned long p2,
unsigned long p3,
unsigned long p4)
{
	CALLBACKPARAM param = (CALLBACKPARAM)pOEMData;
	JNIEnv* env = param->_env;
	jclass  cls = param->_cls;

	char log_buf[256]={0x0};
	sprintf(log_buf,format,p1,p2,p3,p4);
		
	(*env)->CallStaticIntMethod(
		env,
		cls,
		_onLog(env,cls),
		(jlong)param->_core,
		_UTF82JString(env,log_buf)
	);
	return 0;
}

/************************************** jni  **************************************/

static const jint ERR_INVALID_HANDLE = -1;
static const jint ERR_INVALID_INPUT_STR = -2;

/*
 * Class:     com_guobi_gbime_engine_GBIMEV52
 * Method:    newInstance
 * Signature: ()I
 */
jlong JNICALL Java_com_guobi_gbime_engine_GBIMEEngineV52_newInstance
(JNIEnv *env, jclass cls)
{
	GBIMEV5CORE handle = gbimejapi5_impl_new_instance(env,cls);
	if(handle==NULL)
		return ERR_INVALID_HANDLE;
	else
		return (jlong)handle;
}

/*
 * Class:     com_guobi_gbime_engine_GBIMEV52
 * Method:    deleteInstance
 * Signature: (I)V
 */
void JNICALL Java_com_guobi_gbime_engine_GBIMEEngineV52_deleteInstance
(JNIEnv *env, jclass cls, jlong handle)
{
	if(handle<=0)
		return;

	gbimejapi5_impl_delete_instance((GBIMEV5CORE)handle);
}

/*
 * Class:     com_guobi_gbime_engine_GBIMEV52
 * Method:    open
 * Signature: (I[B)I
 */
jint JNICALL Java_com_guobi_gbime_engine_GBIMEEngineV52_open
(JNIEnv *env, jclass cls, jlong handle, jbyteArray ldb)
{
	if(handle<=0)
		return ERR_INVALID_HANDLE;

	jbyte *master_ldb_body = (*env)->GetByteArrayElements(env, ldb, 0);

	jint ret = gbimejapi5_impl_open((GBIMEV5CORE)handle,(GBLPCVOID)master_ldb_body);

	(*env)->ReleaseByteArrayElements(env, ldb, master_ldb_body, 0);

	return ret;
}

/*
 * Class:     com_guobi_gbime_engine_GBIMEV52
 * Method:    openEx
 * Signature: (I[B[B)I
 */
jint JNICALL Java_com_guobi_gbime_engine_GBIMEEngineV52_openEx
  (JNIEnv *env, jclass cls, jlong handle, jbyteArray master_ldb, jbyteArray slave_ldb)
{
	if(handle<=0)
		return ERR_INVALID_HANDLE;

	jbyte *master_ldb_body = (*env)->GetByteArrayElements(env,master_ldb, 0);
	jbyte *slave_ldb_body = (*env)->GetByteArrayElements(env,slave_ldb, 0);

	jint ret = gbimejapi5_impl_open_ex(
		(GBIMEV5CORE)handle,
		(GBLPCVOID)master_ldb_body,
		(GBLPCVOID)slave_ldb_body	
	);

	(*env)->ReleaseByteArrayElements(env, master_ldb, master_ldb_body, 0);
	(*env)->ReleaseByteArrayElements(env, slave_ldb, slave_ldb_body, 0);
	return ret;
}

/*
 * Class:     com_guobi_gbime_engine_GBIMEV52
 * Method:    openWithUDB
 * Signature: (I[B[BI)I
 */
jint JNICALL Java_com_guobi_gbime_engine_GBIMEEngineV52_openWithUDB
  (JNIEnv *env, jclass cls, jlong handle, jbyteArray ldb, jbyteArray udb, jint udb_size )
{
	if(handle<=0)
		return ERR_INVALID_HANDLE;

	jbyte *master_ldb_body = (*env)->GetByteArrayElements(env,ldb, 0);
	jbyte *master_udb_body = (*env)->GetByteArrayElements(env,udb, 0);

	jint ret = gbimejapi5_impl_open_with_udb(
		(GBIMEV5CORE)handle,
		(GBLPCVOID)master_ldb_body,
		(GBLPVOID)master_udb_body,
		udb_size	
	);

	(*env)->ReleaseByteArrayElements(env, ldb, master_ldb_body, 0);
	(*env)->ReleaseByteArrayElements(env, udb, master_udb_body, 0);
	return ret;
}

/*
 * Class:     com_guobi_gbime_engine_GBIMEV52
 * Method:    openExWithUDB
 * Signature: (I[B[BI[B[BI)I
 */
jint JNICALL Java_com_guobi_gbime_engine_GBIMEEngineV52_openExWithUDB
(JNIEnv *env,
 jclass cls,
 jlong handle,
 jbyteArray master_ldb, 
 jbyteArray master_udb,
 jint master_udb_size,
 jbyteArray slave_ldb,
 jbyteArray slave_udb,
 jint slave_udb_size )
{
	if(handle<=0)
		return ERR_INVALID_HANDLE;

	jbyte *master_ldb_body = (*env)->GetByteArrayElements(env,master_ldb, 0);
	jbyte *master_udb_body = (*env)->GetByteArrayElements(env,master_udb, 0);
	jbyte *slave_ldb_body = (*env)->GetByteArrayElements(env,slave_ldb, 0);
	jbyte *slave_udb_body = (*env)->GetByteArrayElements(env,slave_udb, 0);


	jint ret = gbimejapi5_impl_open_ex_with_udb(
		(GBIMEV5CORE)handle,
		(GBLPCVOID)master_ldb_body,
		(GBLPVOID)master_udb_body,
		master_udb_size,
		(GBLPCVOID)slave_ldb_body,
		(GBLPVOID)slave_udb_body,
		slave_udb_size		
	);

	(*env)->ReleaseByteArrayElements(env, master_ldb, master_ldb_body, 0);
	(*env)->ReleaseByteArrayElements(env, master_udb, master_udb_body, 0);
	(*env)->ReleaseByteArrayElements(env, slave_ldb, slave_ldb_body, 0);
	(*env)->ReleaseByteArrayElements(env, slave_udb, slave_udb_body, 0);
	return ret;
}

/*
 * Class:     com_guobi_gbime_engine_GBIMEV52
 * Method:    close
 * Signature: (I)V
 */
void JNICALL Java_com_guobi_gbime_engine_GBIMEEngineV52_close
  (JNIEnv *env, jclass cls, jlong handle)
{
	if(handle<=0)
		return;

	gbimejapi5_impl_close((GBIMEV5CORE)handle);
}

/*
 * Class:     com_guobi_gbime_engine_GBIMEV52
 * Method:    isOpened
 * Signature: (I)Z
 */
jboolean JNICALL Java_com_guobi_gbime_engine_GBIMEEngineV52_isOpened
  (JNIEnv *env, jclass cls, jlong handle)
{
	if(handle<=0)
		return JNI_FALSE;
	
	return gbimejapi5_impl_is_opened((GBIMEV5CORE)handle)?JNI_TRUE:JNI_FALSE;
}

/*
 * Class:     com_guobi_gbime_engine_GBIMEV52
 * Method:    enableUpdateCallback
 * Signature: (IZ)V
 */
void JNICALL Java_com_guobi_gbime_engine_GBIMEEngineV52_enableUpdateCallback
  (JNIEnv *env, jclass cls, jlong handle, jboolean enable)
{
	if(handle<=0)
		return;

	gbimejapi5_impl_enable_udb_update_callback((GBIMEV5CORE)handle,enable?_fpGBUDBUpdate:NULL);
}

/*
 * Class:     com_guobi_gbime_engine_GBIMEV52
 * Method:    setUDBUpdateCallbackFreq
 * Signature: (II)I
 */
jint JNICALL Java_com_guobi_gbime_engine_GBIMEEngineV52_setUDBUpdateCallbackFreq
  (JNIEnv *env, jclass cls, jlong handle, jint update_freq)
{
	if(handle<=0)
		return ERR_INVALID_HANDLE;
	
	return  gbimejapi5_impl_set_udb_update_freq((GBIMEV5CORE)handle,update_freq);
}

/*
 * Class:     com_guobi_gbime_engine_GBIMEV52
 * Method:    clearAllUDBData
 * Signature: (I)V
 */
void JNICALL Java_com_guobi_gbime_engine_GBIMEEngineV52_clearAllUDBData
  (JNIEnv *env, jclass cls, jlong handle)
{
	if(handle<=0)
		return;

	gbimejapi5_impl_clear_udb_data((GBIMEV5CORE)handle);
}

/*
 * Class:     com_guobi_gbime_engine_GBIMEV52
 * Method:    deleteUDBPhrase
 * Signature: (ILjava/lang/String;)I
 */
jint JNICALL Java_com_guobi_gbime_engine_GBIMEEngineV52_deleteUDBPhrase
  (JNIEnv *env, jclass cls, jlong handle, jstring str)
{
	if(handle<=0)
		return ERR_INVALID_HANDLE;
	
	unsigned short* phrase = _JString2UCS(env,str);
	if(phrase==NULL)
		return ERR_INVALID_INPUT_STR;

	GBINT ret = gbimejapi5_impl_delete_udb_phrase((GBIMEV5CORE)handle,(GBLPCWCHAR)phrase);

	free(phrase);
	return ret;
}

/*
 * Class:     com_guobi_gbime_engine_GBIMEV52
 * Method:    startEnumerateUDBPhrase
 * Signature: (I)V
 */
void JNICALL Java_com_guobi_gbime_engine_GBIMEEngineV52_startEnumerateUDBPhrase
  (JNIEnv *env, jclass cls, jlong handle)
{
	if(handle<=0)
		return;

	gbimejapi5_impl_start_enum_udb_phrase((GBIMEV5CORE)handle);
}

/*
 * Class:     com_guobi_gbime_engine_GBIMEV52
 * Method:    enumerateNextUDBPhrase
 * Signature: (I)Ljava/lang/String;
 */
jstring JNICALL Java_com_guobi_gbime_engine_GBIMEEngineV52_enumerateNextUDBPhrase
  (JNIEnv *env, jclass cls, jlong handle)
{
	if(handle<=0)
		return NULL;
	
	GBWCHAR pBuffer[256]={0x0000};
	if(gbimejapi5_impl_get_next_udb_phrase((GBIMEV5CORE)handle,pBuffer,sizeof(pBuffer))!=GBOK)
		return NULL;
	
	return _UCS2JString(env,pBuffer);
}

/*
 * Class:     com_guobi_gbime_engine_GBIMEV52
 * Method:    enablePagingCallback
 * Signature: (IZ)V
 */
void JNICALL Java_com_guobi_gbime_engine_GBIMEEngineV52_enablePagingCallback
  (JNIEnv *env, jclass cls, jlong handle, jboolean enable)
{
	if(handle<=0)
		return;

	gbimejapi5_impl_enable_paging_callback((GBIMEV5CORE)handle,enable?_fpGetWStringWidth:NULL);
}

/*
 * Class:     com_guobi_gbime_engine_GBIMEV52
 * Method:    setAlpPagingOptions
 * Signature: (IIII)I
 */
jint JNICALL Java_com_guobi_gbime_engine_GBIMEEngineV52_setAlpPagingOptions
  (JNIEnv *env, jclass cls, jlong handle, jint page_width, jint word_space, jint max_cand)
{
	if(handle<=0)
		return ERR_INVALID_HANDLE;

	return gbimejapi5_impl_set_alp_paging_options((GBIMEV5CORE)handle,page_width,word_space,max_cand);
}

/*
 * Class:     com_guobi_gbime_engine_GBIMEV52
 * Method:    setChnPagingOptions
 * Signature: (IIII)I
 */
jint JNICALL Java_com_guobi_gbime_engine_GBIMEEngineV52_setChnPagingOptions
  (JNIEnv *env, jclass cls, jlong handle, jint page_width, jint word_space, jint max_cand)
{
	if(handle<=0)
		return ERR_INVALID_HANDLE;

	return gbimejapi5_impl_set_chn_paging_options((GBIMEV5CORE)handle,page_width,word_space,max_cand);
}

/*
 * Class:     com_guobi_gbime_engine_GBIMEV52
 * Method:    setSyllPagingOptions
 * Signature: (IIII)I
 */
jint JNICALL Java_com_guobi_gbime_engine_GBIMEEngineV52_setSyllPagingOptions
  (JNIEnv *env, jclass cls, jlong handle, jint page_width, jint word_space, jint max_cand)
{
	if(handle<=0)
		return ERR_INVALID_HANDLE;

	return gbimejapi5_impl_set_syll_paging_options((GBIMEV5CORE)handle,page_width,word_space,max_cand);
}

/*
 * Class:     com_guobi_gbime_engine_GBIMEV52
 * Method:    setInputMode
 * Signature: (III)I
 */
jint JNICALL Java_com_guobi_gbime_engine_GBIMEEngineV52_setInputMode
  (JNIEnv *env, jclass cls, jlong handle, jint input_mode, jint cand_type)
{
	if(handle<=0)
		return ERR_INVALID_HANDLE;

	return gbimejapi5_impl_set_input_mode((GBIMEV5CORE)handle,input_mode,cand_type);
}

/*
 * Class:     com_guobi_gbime_engine_GBIMEV52
 * Method:    inputEvent
 * Signature: (II)V
 */
jint JNICALL Java_com_guobi_gbime_engine_GBIMEEngineV52_inputEvent
  (JNIEnv *env, jclass cls, jlong handle, jint evt)
{
	if(handle<=0)
		return ERR_INVALID_HANDLE;

	return gbimejapi5_impl_input_event((GBIMEV5CORE)handle,evt);
}

/*
 * Class:     com_guobi_gbime_engine_GBIMEV52
 * Method:    inputEventEx
 * Signature: (IIJ)V
 */
jint JNICALL Java_com_guobi_gbime_engine_GBIMEEngineV52_inputEventEx
  (JNIEnv *env, jclass cls, jlong handle, jint evt, jlong param)
{
	if(handle<=0)
		return ERR_INVALID_HANDLE;
	
	return gbimejapi5_impl_input_event_ex((GBIMEV5CORE)handle,evt,param);
}

/*
 * Class:     com_guobi_gbime_engine_GBIMEV52
 * Method:    inputDefaultInterpuction
 * Signature: (I)V
 */
jint  JNICALL Java_com_guobi_gbime_engine_GBIMEEngineV52_inputDefaultInterpuction
  (JNIEnv *env, jclass cls, jlong handle)
{
	if(handle<=0)
		return ERR_INVALID_HANDLE;

	return gbimejapi5_impl_input_default_interpuction((GBIMEV5CORE)handle);
}

/*
 * Class:     com_guobi_gbime_engine_GBIMEV52
 * Method:    clearAllInput
 * Signature: (I)V
 */
void JNICALL Java_com_guobi_gbime_engine_GBIMEEngineV52_clearAllInput
  (JNIEnv *env, jclass cls, jlong handle)
{
	if(handle<=0)
		return;

	gbimejapi5_impl_clear_all_input((GBIMEV5CORE)handle);
}

/*
 * Class:     com_guobi_gbime_engine_GBIMEV52
 * Method:    hasCandidate
 * Signature: (I)Z
 */
jboolean JNICALL Java_com_guobi_gbime_engine_GBIMEEngineV52_hasCandidate
  (JNIEnv *env, jclass cls, jlong handle)
{
	if(handle<=0)
		return JNI_FALSE;

	return gbimejapi5_impl_has_candidate((GBIMEV5CORE)handle)?JNI_TRUE:JNI_FALSE;
}

/*
 * Class:     com_guobi_gbime_engine_GBIMEV52
 * Method:    getCandCountInPage
 * Signature: (I)I
 */
jint JNICALL Java_com_guobi_gbime_engine_GBIMEEngineV52_getCandCountInPage
  (JNIEnv *env, jclass cls, jlong handle)
{
	if(handle<=0)
		return ERR_INVALID_HANDLE;

	return gbimejapi5_impl_get_cand_count_in_page((GBIMEV5CORE)handle);
}

/*
 * Class:     com_guobi_gbime_engine_GBIMEV52
 * Method:    hasPrevCandPage
 * Signature: (I)Z
 */
jboolean JNICALL Java_com_guobi_gbime_engine_GBIMEEngineV52_hasPrevCandPage
  (JNIEnv *env, jclass cls, jlong handle)
{
	if(handle<=0)
		return JNI_FALSE;

	return gbimejapi5_impl_has_prev_cand_page((GBIMEV5CORE)handle)?JNI_TRUE:JNI_FALSE;
}

/*
 * Class:     com_guobi_gbime_engine_GBIMEV52
 * Method:    hasNextCandPage
 * Signature: (I)Z
 */
jboolean JNICALL Java_com_guobi_gbime_engine_GBIMEEngineV52_hasNextCandPage
  (JNIEnv *env, jclass cls, jlong handle)
{
	if(handle<=0)
		return JNI_FALSE;

	return gbimejapi5_impl_has_next_cand_page((GBIMEV5CORE)handle)?JNI_TRUE:JNI_FALSE;
}

/*
 * Class:     com_guobi_gbime_engine_GBIMEV52
 * Method:    movePrevCandPage
 * Signature: (I)Z
 */
jboolean JNICALL Java_com_guobi_gbime_engine_GBIMEEngineV52_movePrevCandPage
  (JNIEnv *env, jclass cls, jlong handle)
{
	if(handle<=0)
		return JNI_FALSE;

	return gbimejapi5_impl_move_prev_cand_page((GBIMEV5CORE)handle)?JNI_TRUE:JNI_FALSE;
}

/*
 * Class:     com_guobi_gbime_engine_GBIMEV52
 * Method:    moveNextCandPage
 * Signature: (I)Z
 */
jboolean JNICALL Java_com_guobi_gbime_engine_GBIMEEngineV52_moveNextCandPage
  (JNIEnv *env, jclass cls, jlong handle)
{
	if(handle<=0)
		return JNI_FALSE;

	return gbimejapi5_impl_move_next_cand_page((GBIMEV5CORE)handle)?JNI_TRUE:JNI_FALSE;
}

/*
 * Class:     com_guobi_gbime_engine_GBIMEV52
 * Method:    getCandInPageByIdx
 * Signature: (II)Ljava/lang/String;
 */
jstring JNICALL Java_com_guobi_gbime_engine_GBIMEEngineV52_getCandInPageByIdx
  (JNIEnv *env, jclass cls, jlong handle, jint idx)
{
	if(handle<=0)
		return NULL;

	return _UCS2JString(env,gbimejapi5_impl_get_cand_in_page_by_idx((GBIMEV5CORE)handle,idx));
}

/*
 * Class:     com_guobi_gbime_engine_GBIMEV52
 * Method:    getCandInputLenInPageByIdx
 * Signature: (JI)I
 */
jint JNICALL Java_com_guobi_gbime_engine_GBIMEEngineV52_getCandInputLenInPageByIdx
  (JNIEnv *env, jclass cls, jlong handle, jint idx)
{
	if(handle<=0)
		return ERR_INVALID_HANDLE;

	return gbimejapi5_impl_get_cand_input_len_in_page_by_idx((GBIMEV5CORE)handle,idx);
}

/*
 * Class:     com_guobi_gbime_engine_GBIMEV52
 * Method:    selectCandInPageByIdx
 * Signature: (II)V
 */
void JNICALL Java_com_guobi_gbime_engine_GBIMEEngineV52_selectCandInPageByIdx
  (JNIEnv *env, jclass cls, jlong handle, jint idx)
{
	if(handle<=0)
		return;

	gbimejapi5_impl_select_cand_in_page_by_idx((GBIMEV5CORE)handle,idx);
}

/*
 * Class:     com_guobi_gbime_engine_GBIMEV52
 * Method:    hasSyllable
 * Signature: (I)Z
 */
jboolean JNICALL Java_com_guobi_gbime_engine_GBIMEEngineV52_hasSyllable
  (JNIEnv *env, jclass cls, jlong handle)
{
	if(handle<=0)
		return JNI_FALSE;
	
	return gbimejapi5_impl_has_syllable((GBIMEV5CORE)handle)?JNI_TRUE:JNI_FALSE;
}

/*
 * Class:     com_guobi_gbime_engine_GBIMEV52
 * Method:    getSyllCountInPage
 * Signature: (I)I
 */
jint JNICALL Java_com_guobi_gbime_engine_GBIMEEngineV52_getSyllCountInPage
  (JNIEnv *env, jclass cls, jlong handle)
{
	if(handle<=0)
		return ERR_INVALID_HANDLE;

	return gbimejapi5_impl_get_syll_count_in_page((GBIMEV5CORE)handle);
}

/*
 * Class:     com_guobi_gbime_engine_GBIMEV52
 * Method:    hasPrevSyllPage
 * Signature: (I)Z
 */
jboolean JNICALL Java_com_guobi_gbime_engine_GBIMEEngineV52_hasPrevSyllPage
  (JNIEnv *env, jclass cls, jlong handle)
{
	if(handle<=0)
		return JNI_FALSE;

	return gbimejapi5_impl_has_prev_syll_page((GBIMEV5CORE)handle)?JNI_TRUE:JNI_FALSE;
}

/*
 * Class:     com_guobi_gbime_engine_GBIMEV52
 * Method:    hasNextSyllPage
 * Signature: (I)Z
 */
jboolean JNICALL Java_com_guobi_gbime_engine_GBIMEEngineV52_hasNextSyllPage
  (JNIEnv *env, jclass cls, jlong handle)
{
	if(handle<=0)
		return JNI_FALSE;

	return gbimejapi5_impl_has_next_syll_page((GBIMEV5CORE)handle)?JNI_TRUE:JNI_FALSE;
}

/*
 * Class:     com_guobi_gbime_engine_GBIMEV52
 * Method:    movePrevSyllPage
 * Signature: (I)Z
 */
jboolean JNICALL Java_com_guobi_gbime_engine_GBIMEEngineV52_movePrevSyllPage
  (JNIEnv *env, jclass cls, jlong handle)
{
	if(handle<=0)
		return JNI_FALSE;

	return gbimejapi5_impl_move_prev_syll_page((GBIMEV5CORE)handle)?JNI_TRUE:JNI_FALSE;
}

/*
 * Class:     com_guobi_gbime_engine_GBIMEV52
 * Method:    moveNextSyllPage
 * Signature: (I)Z
 */
jboolean JNICALL Java_com_guobi_gbime_engine_GBIMEEngineV52_moveNextSyllPage
  (JNIEnv *env, jclass cls, jlong handle)
{
	if(handle<=0)
		return JNI_FALSE;

	return gbimejapi5_impl_move_next_syll_page((GBIMEV5CORE)handle)?JNI_TRUE:JNI_FALSE;
}

/*
 * Class:     com_guobi_gbime_engine_GBIMEV52
 * Method:    getSyllInPageByIdx
 * Signature: (II)Ljava/lang/String;
 */
jstring JNICALL Java_com_guobi_gbime_engine_GBIMEEngineV52_getSyllInPageByIdx
  (JNIEnv *env, jclass cls, jlong handle, jint idx)
{
	if(handle<=0)
		return NULL;

	return _UCS2JString(env,gbimejapi5_impl_get_syll_in_page_by_idx((GBIMEV5CORE)handle,idx));
}

/*
 * Class:     com_guobi_gbime_engine_GBIMEV52
 * Method:    selectSyllInPageByIdx
 * Signature: (II)V
 */
void JNICALL Java_com_guobi_gbime_engine_GBIMEEngineV52_selectSyllInPageByIdx
  (JNIEnv *env, jclass cls, jlong handle, jint idx)
{
	if(handle<=0)
		return;

	gbimejapi5_impl_select_syll_in_page_by_idx((GBIMEV5CORE)handle,idx);
}

/*
 * Class:     com_guobi_gbime_engine_GBIMEV52
 * Method:    getSelectedSyllIdx
 * Signature: (I)I
 */
jint JNICALL Java_com_guobi_gbime_engine_GBIMEEngineV52_getSelectedSyllIdx
  (JNIEnv *env, jclass cls, jlong handle)
{
	if(handle<=0)
		return ERR_INVALID_HANDLE;

	return gbimejapi5_impl_get_selected_syll_Idx((GBIMEV5CORE)handle);
}

/*
 * Class:     com_guobi_gbime_engine_GBIMEV52
 * Method:    getInputString
 * Signature: (I)Ljava/lang/String;
 */
jstring JNICALL Java_com_guobi_gbime_engine_GBIMEEngineV52_getInputString
  (JNIEnv *env, jclass cls, jlong handle)
{
	if(handle<=0)
		return NULL;

	return _UCS2JString(env,gbimejapi5_impl_get_input_string((GBIMEV5CORE)handle));
}

/*
 * Class:     com_guobi_gbime_engine_GBIMEV52
 * Method:    getOutputString
 * Signature: (I)Ljava/lang/String;
 */
jstring JNICALL Java_com_guobi_gbime_engine_GBIMEEngineV52_getOutputString
  (JNIEnv *env, jclass cls, jlong handle)
{
	if(handle<=0)
		return NULL;

	return _UCS2JString(env,gbimejapi5_impl_get_output_string((GBIMEV5CORE)handle));
}

/*
 * Class:     com_guobi_gbime_engine_GBIMEV52
 * Method:    getUpScreenString
 * Signature: (I)Ljava/lang/String;
 */
jstring JNICALL Java_com_guobi_gbime_engine_GBIMEEngineV52_getUpScreenString
  (JNIEnv *env, jclass cls, jlong handle)
{
	if(handle<=0)
		return NULL;

	return _UCS2JString(env,gbimejapi5_impl_get_upscreen_string((GBIMEV5CORE)handle));
}

/*
 * Class:     com_guobi_gbime_engine_GBIMEV52
 * Method:    clearUpScreenString
 * Signature: (I)V
 */
void JNICALL Java_com_guobi_gbime_engine_GBIMEEngineV52_clearUpScreenString
  (JNIEnv *env, jclass cls, jlong handle)
{
	if(handle<=0)
		return;

	gbimejapi5_impl_clear_upscreen_string((GBIMEV5CORE)handle);
}

/*
 * Class:     com_guobi_gbime_engine_GBIMEV52
 * Method:    getStorkeString
 * Signature: (I)Ljava/lang/String;
 */
jstring JNICALL Java_com_guobi_gbime_engine_GBIMEEngineV52_getStorkeString
  (JNIEnv *env, jclass cls, jlong handle)
{
	if(handle<=0)
		return NULL;

	return _UCS2JString(env,gbimejapi5_impl_get_stroke_string((GBIMEV5CORE)handle));
}

/*
 * Class:     com_guobi_gbime_engine_GBIMEV52
 * Method:    getComponentsCount
 * Signature: (I)I
 */
jint JNICALL Java_com_guobi_gbime_engine_GBIMEEngineV52_getComponentsCount
  (JNIEnv *env, jclass cls, jlong handle)
{
	if(handle<=0)
		return ERR_INVALID_HANDLE;

	return gbimejapi5_impl_get_components_count((GBIMEV5CORE)handle);
}

/*
 * Class:     com_guobi_gbime_engine_GBIMEV52
 * Method:    getComponentStringByIdx
 * Signature: (II)Ljava/lang/String;
 */
jstring JNICALL Java_com_guobi_gbime_engine_GBIMEEngineV52_getComponentStringByIdx
  (JNIEnv *env, jclass cls, jlong handle, jint idx)
{
	return NULL;
}

/*
 * Class:     com_guobi_gbime_engine_GBIMEV52
 * Method:    getComponentUnicodeByIdx
 * Signature: (II)I
 */
jint JNICALL Java_com_guobi_gbime_engine_GBIMEEngineV52_getComponentUnicodeByIdx
  (JNIEnv *env, jclass cls, jlong handle, jint idx)
{
	if(handle<=0)
		return ERR_INVALID_HANDLE;

	return gbimejapi5_impl_get_component_unicode_by_idx((GBIMEV5CORE)handle,idx);
}

/*
 * Class:     com_guobi_gbime_engine_GBIMEV52
 * Method:    setAssocWord
 * Signature: (ILjava/lang/String;Z)I
 */
jint JNICALL Java_com_guobi_gbime_engine_GBIMEEngineV52_setAssocWord
  (JNIEnv *env, jclass cls, jlong handle, jstring str, jboolean reset)
{
	if(handle<=0)
		return ERR_INVALID_HANDLE;
	
	unsigned short* word = _JString2UCS(env,str);
	if(word==NULL)
		return ERR_INVALID_INPUT_STR;

	jint ret=gbimejapi5_impl_set_assocword((GBIMEV5CORE)handle,word,reset);
	
	free(word);
	return ret;
}

/*
 * Class:     com_guobi_gbime_engine_GBIMEV52
 * Method:    setCapState
 * Signature: (IIIZ)I
 */
jint JNICALL Java_com_guobi_gbime_engine_GBIMEEngineV52_setCapState
  (JNIEnv *env, 
 jclass cls,
 jlong handle,
 jint shift_down,
 jint capslock_toggle,
 jboolean bUpdateCandidates)
{
	if(handle<=0)
		return ERR_INVALID_HANDLE;

	return gbimejapi5_impl_set_capstate((GBIMEV5CORE)handle,shift_down,capslock_toggle,bUpdateCandidates);
}

/*
 * Class:     com_guobi_gbime_engine_GBIMEV52
 * Method:    setFuzzyOptions
 * Signature: (IJZ)I
 */
jint JNICALL Java_com_guobi_gbime_engine_GBIMEEngineV52_setFuzzyOptions
  (JNIEnv *env, jclass cls, jlong handle, jlong config, jboolean on)
{
	if(handle<=0)
		return ERR_INVALID_HANDLE;

	return gbimejapi5_impl_set_fuzzy_options((GBIMEV5CORE)handle,config,on);
}

/*
 * Class:     com_guobi_gbime_engine_GBIMEV52
 * Method:    switchToSlaveLang
 * Signature: (II)I
 */
jint JNICALL Java_com_guobi_gbime_engine_GBIMEEngineV52_switchToSlaveLang
  (JNIEnv *env, jclass cls, jlong handle, jint input_mode)
{
	if(handle<=0)
		return ERR_INVALID_HANDLE;

	return gbimejapi5_impl_engine_switchto_slave_lang((GBIMEV5CORE)handle,input_mode);
}

/*
 * Class:     com_guobi_gbime_engine_GBIMEV52
 * Method:    setEngineOption
 * Signature: (IIJJ)I
 */
jint JNICALL Java_com_guobi_gbime_engine_GBIMEEngineV52_setEngineOption
  (JNIEnv *env, jclass cls, jlong handle, jint engine_id, jlong config, jlong option)
{
	if(handle<=0)
		return ERR_INVALID_HANDLE;

	return gbimejapi5_impl_set_engine_option((GBIMEV5CORE)handle,engine_id,config,option);
}

/*
 * Class:     com_guobi_gbime_engine_GBIMEV52
 * Method:    getCurState
 * Signature: (I)I
 */
jint JNICALL Java_com_guobi_gbime_engine_GBIMEEngineV52_getCurState
  (JNIEnv *env, jclass cls, jlong handle)
{
	if(handle<=0)
		return ERR_INVALID_HANDLE;

	return gbimejapi5_impl_get_cur_state((GBIMEV5CORE)handle);
}

/*
 * Class:     com_guobi_gbime_engine_GBIMEV52
 * Method:    enableLog
 * Signature: (JZ)V
 */
void JNICALL Java_com_guobi_gbime_engine_GBIMEEngineV52_enableLog
  (JNIEnv *env, jclass cls, jlong handle, jboolean enable)
{
	if(handle<=0)
		return;

	gbimejapi5_impl_enable_log((GBIMEV5CORE)handle,enable?_fpLogRecorder:NULL);
}


jint JNICALL Java_com_guobi_gbime_engine_GBIMEEngineV52_registerInterpunctionKeyEx
  (	JNIEnv *env, 
	jclass cls, 
	jlong handle, 
	jint key, 
	jstring str,
	jint cSplit, 
	jint nCandListType,
	jint nCandDesireRow,
	jint nCandCountPerRow,
	jint nOption )
{
	if(handle<=0)
		return ERR_INVALID_HANDLE;

	unsigned short* symbols = _JString2UCS(env,str);
	if(symbols==NULL)
		return ERR_INVALID_INPUT_STR;

	jint ret=gbimejapi5_impl_registerInterpunctionKeyEx(
		(GBIMEV5CORE)handle,
		key,		
		symbols,
		cSplit,
		nCandListType,
		nCandDesireRow,
		nCandCountPerRow,
		nOption
	);
	
	if(ret!=GBOK)
		free(symbols);
	return ret;
}

/*
 * Class:     com_guobi_gbime_engine_GBIMEV52
 * Method:    selectCandEx
 * Signature: (JLjava/lang/String;I)I
 */
jint JNICALL Java_com_guobi_gbime_engine_GBIMEEngineV52_selectCandEx
  (JNIEnv *env, jclass cls, jlong handle, jstring cand, jint input_len)
{
	if(handle<=0)
		return ERR_INVALID_HANDLE;
	
	unsigned short* word = _JString2UCS(env,cand);
	if(word==NULL)
		return ERR_INVALID_INPUT_STR;

	jint ret = gbimejapi5_impl_select_cand_ex((GBIMEV5CORE)handle,word,input_len);

	free(word);
	return ret;
}

jint JNICALL Java_com_guobi_gbime_engine_GBIMEEngineV52_addUDBPhrase
  (JNIEnv *env, jclass cls, jlong handle, jstring word, jstring syll)

{
	if(handle<=0)
		return ERR_INVALID_HANDLE;

	unsigned short* w = _JString2UCS(env,word);
	if(w==NULL)
		return ERR_INVALID_INPUT_STR;

	unsigned short* s = _JString2UCS(env,syll);


	jint ret = gbimejapi5_impl_add_udb_phrase((GBIMEV5CORE)handle,w,s);

	free(w);
	if(s!=NULL)
		free(s);
	return ret;
}

/*
 * Class:     com_guobi_gbime_engine_GBIMEV52
 * Method:    getPinyinCode
 * Signature: (JLjava/lang/String;I)Ljava/lang/String;
 */
jstring JNICALL Java_com_guobi_gbime_engine_GBIMEEngineV52_getPinyinCode
  (JNIEnv *env, jclass cls, jlong handle, jstring word, jint idx)
{
	if(handle<=0)
		return NULL;

	unsigned short* w = _JString2UCS(env,word);
	if(w==NULL)
		return NULL;

	return _UCS2JString(env,gbimejapi5_impl_get_pinyin_code((GBIMEV5CORE)handle,w,idx));
}

/*
 * Class:     com_guobi_gbime_engine_GBIMEV52
 * Method:    rewindCandPage
 * Signature: (J)V
 */
void JNICALL Java_com_guobi_gbime_engine_GBIMEEngineV52_rewindCandPage
  (JNIEnv *env, jclass cls, jlong handle)
{
	if(handle<=0)
		return;

	gbimejapi5_impl_update_candidates((GBIMEV5CORE)handle);
}
/************************************** jni init **************************************/



