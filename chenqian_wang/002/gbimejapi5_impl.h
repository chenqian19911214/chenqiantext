/*
 *
 *
 */

#ifndef __GBIMEJAPI5_IMPL_H__
#define __GBIMEJAPI5_IMPL_H__
#ifdef __cplusplus
extern "C" {
#endif

#ifndef WIN32
#include <jni.h>
#endif
#include "gbtype.h"
#include "gbapi.h"
#include "gbim.h"
#include "gbdef.h"

typedef struct _GBIMEV5CORE* GBIMEV5CORE;


struct _CALLBACKPARAM
{
#ifndef WIN32
	JNIEnv* _env;
	jclass _cls;
#endif
	GBIMEV5CORE _core;
	
};
typedef struct _CALLBACKPARAM* CALLBACKPARAM;

/**/
GBIMEV5CORE gbimejapi5_impl_new_instance(
#ifndef WIN32
	JNIEnv *env, jclass cls
#endif
	);

/**/
void gbimejapi5_impl_delete_instance(GBIMEV5CORE core);

/**/
void gbimejapi5_impl_close(GBIMEV5CORE core);

/**/
GBBOOL gbimejapi5_impl_is_opened(GBIMEV5CORE core);

/**/
GBRETURN gbimejapi5_impl_open(GBIMEV5CORE core,GBLPCVOID master_ldb);

/**/
GBRETURN gbimejapi5_impl_open_ex(GBIMEV5CORE core,GBLPCVOID master_ldb,GBLPCVOID slave_ldb);

/**/
GBRETURN gbimejapi5_impl_open_with_udb(GBIMEV5CORE core,GBLPCVOID master_ldb,GBLPVOID master_udb,GBINT master_udb_size);

/**/
GBRETURN gbimejapi5_impl_open_ex_with_udb(
GBIMEV5CORE core,
GBLPCVOID master_ldb,
GBLPVOID master_udb,
GBINT master_udb_size,
GBLPCVOID slave_ldb,
GBLPVOID slave_udb,
GBINT slave_udb_size);

/**/
void gbimejapi5_impl_enable_udb_update_callback(GBIMEV5CORE core,fpGBUDBUpdate_t fpUpdateFunction);

/**/
GBRETURN gbimejapi5_impl_set_udb_update_freq(GBIMEV5CORE core,GBINT freq);

/**/
void gbimejapi5_impl_clear_udb_data(GBIMEV5CORE core);

/**/
GBRETURN gbimejapi5_impl_delete_udb_phrase(GBIMEV5CORE core,GBLPCWCHAR phrase);

/**/
GBRETURN gbimejapi5_impl_add_udb_phrase(GBIMEV5CORE core,GBLPCWCHAR phrase,GBLPCWCHAR syll);


/**/
void gbimejapi5_impl_start_enum_udb_phrase(GBIMEV5CORE core);

/**/
GBRETURN gbimejapi5_impl_get_next_udb_phrase(GBIMEV5CORE core,GBLPWCHAR pBuffer,GBINT nBuffLen);

/**/
void gbimejapi5_impl_enable_paging_callback(GBIMEV5CORE core,fpGetWStringWidth_t fpGetWStringWidth);

/**/
GBRETURN gbimejapi5_impl_set_alp_paging_options(GBIMEV5CORE core,GBINT page_width,GBINT word_space,GBINT max_cand_per_page);

/**/
GBRETURN gbimejapi5_impl_set_chn_paging_options(GBIMEV5CORE core,GBINT page_width,GBINT word_space,GBINT max_cand_per_page);

/**/
GBRETURN gbimejapi5_impl_set_syll_paging_options(GBIMEV5CORE core,GBINT page_width,GBINT word_space,GBINT max_cand_per_page);

/**/
GBRETURN gbimejapi5_impl_set_input_mode(GBIMEV5CORE core, GBINT input_mode,GBINT cand_type);

/**/
GBINT gbimejapi5_impl_get_cur_state(GBIMEV5CORE core);

/**/
GBRETURN gbimejapi5_impl_input_event(GBIMEV5CORE core, GBINT evt);

/**/
GBRETURN gbimejapi5_impl_input_event_ex(GBIMEV5CORE core, GBINT evt,GBU32 param);

/**/
GBRETURN gbimejapi5_impl_input_default_interpuction(GBIMEV5CORE core);

/**/
void gbimejapi5_impl_clear_all_input(GBIMEV5CORE core);

/**/
GBBOOL gbimejapi5_impl_has_candidate(GBIMEV5CORE core);

/**/
GBINT gbimejapi5_impl_get_cand_count_in_page(GBIMEV5CORE core);

/**/
GBINT gbimejapi5_impl_has_prev_cand_page(GBIMEV5CORE core);

/**/
GBINT gbimejapi5_impl_has_next_cand_page(GBIMEV5CORE core);

/**/
GBBOOL gbimejapi5_impl_move_prev_cand_page(GBIMEV5CORE core);

/**/
GBBOOL gbimejapi5_impl_move_next_cand_page(GBIMEV5CORE core);

/**/
GBLPCWCHAR gbimejapi5_impl_get_cand_in_page_by_idx(GBIMEV5CORE core,GBINT idx);

/**/
GBBOOL gbimejapi5_impl_has_syllable(GBIMEV5CORE core);

/**/
GBINT gbimejapi5_impl_get_syll_count_in_page(GBIMEV5CORE core);

/**/
GBBOOL gbimejapi5_impl_has_prev_syll_page(GBIMEV5CORE core);

/**/
GBBOOL gbimejapi5_impl_has_next_syll_page(GBIMEV5CORE core);

/**/
GBBOOL gbimejapi5_impl_move_prev_syll_page(GBIMEV5CORE core);

/**/
GBBOOL gbimejapi5_impl_move_next_syll_page(GBIMEV5CORE core);

/**/
GBLPCWCHAR  gbimejapi5_impl_get_syll_in_page_by_idx(GBIMEV5CORE core,GBINT idx);

/**/
void gbimejapi5_impl_select_syll_in_page_by_idx(GBIMEV5CORE core,GBINT idx);

/**/
GBINT gbimejapi5_impl_get_selected_syll_Idx(GBIMEV5CORE core);

/**/
GBLPCWCHAR  gbimejapi5_impl_get_input_string(GBIMEV5CORE core);

/**/
GBLPCWCHAR  gbimejapi5_impl_get_output_string(GBIMEV5CORE core);

/**/
GBLPCWCHAR  gbimejapi5_impl_get_upscreen_string(GBIMEV5CORE core);

/**/
void gbimejapi5_impl_clear_upscreen_string(GBIMEV5CORE core);

/**/
GBLPCWCHAR gbimejapi5_impl_get_stroke_string(GBIMEV5CORE core);

/**/
GBINT gbimejapi5_impl_get_components_count(GBIMEV5CORE core);

/**/
unsigned short gbimejapi5_impl_get_component_unicode_by_idx(GBIMEV5CORE core,GBINT idx);

/**/
GBRETURN gbimejapi5_impl_set_assocword(GBIMEV5CORE core, GBLPCWCHAR strConfirmedWord,GBBOOL reset);

/**/
GBRETURN gbimejapi5_impl_set_capstate(GBIMEV5CORE core,GBINT bShiftDown,GBINT bCapsLockToggle,GBBOOL bUpdateCandidates);

/**/
GBRETURN gbimejapi5_impl_set_fuzzy_options(GBIMEV5CORE core, GBU32 config,GBINT on );

/**/
GBRETURN gbimejapi5_impl_set_engine_option(GBIMEV5CORE core,GBINT engine_id,GBU32 config,GBU32 option);

/**/
GBRETURN gbimejapi5_impl_engine_switchto_slave_lang( GBIMEV5CORE core, GBINT input_mode );

/**/
void gbimejapi5_impl_enable_log(GBIMEV5CORE core,fpLogRecorder_t fpLogRecorder);

/**/
GBRETURN gbimejapi5_impl_registerInterpunctionKeyEx(
	GBIMEV5CORE core,
	GBINT cKey,
	GBLPWCHAR pSymbols,
	GBINT cSplit,
	GBINT nCandListType,
	GBINT nCandDesireRow,
	GBINT nCandCountPerRow,
	GBUINT nOption
);

/**/
void gbimejapi5_impl_select_cand_in_page_by_idx(GBIMEV5CORE core,GBINT idx);

/**/
GBRETURN gbimejapi5_impl_select_cand_ex(GBIMEV5CORE core,GBLPCWCHAR cand,GBINT input_len);

/**/
GBINT gbimejapi5_impl_get_cand_input_len_in_page_by_idx(GBIMEV5CORE core,GBINT idx);

/**/
GBRETURN gbimejapi5_impl_attach_mdb(GBIMEV5CORE core,GBLPCVOID mdb, GBINT mdb_len,GBINT lang,GBINT* pID);

/**/
GBRETURN gbimejapi5_impl_detach_mdb(GBIMEV5CORE core,GBINT ID);

/**/
GBLPCWCHAR gbimejapi5_impl_get_pinyin_code(GBIMEV5CORE core,GBLPCWCHAR word,GBINT idx);

GBRETURN gbimejapi5_impl_update_candidates(GBIMEV5CORE core);

#ifdef __cplusplus
}
#endif
#endif //__GBIMEJAPI5_IMPL_H__


