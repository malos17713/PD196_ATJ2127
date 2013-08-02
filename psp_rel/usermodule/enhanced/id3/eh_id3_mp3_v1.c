/*******************************************************************************
 *                              US212A
 *                            Module: ENHANCED
 *                 Copyright(c) 2003-2011 Actions Semiconductor,
 *                            All Rights Reserved.
 *
 * History:
 *      <author>    <time>           <version >             <desc>
 *       liminxian  2011-9-15 15:37    1.0             build this file
 *******************************************************************************/
/*!
 * \file     eh_id3_mp3_v1.c
 * \brief    这里填写文件的概述
 * \author   liminxian
 * \par      GENERAL DESCRIPTION:
 *               这里对文件进行描述
 * \par      EXTERNALIZED FUNCTIONS:
 *               这里描述调用到外面的模块
 * \version 1.0
 * \date  2011/9/15
 *******************************************************************************/

#include "eh_id3.h"

static const uint16 TabAddr[] =
{ 0, 5, 16, 23, 28, 33, 37, 43, 50, 54, 59, 65, 71, 76, 79, 82, 85, 91, 95, 101, 111, 122, 125, 135, 141, 151, 162,
        169, 177, 182, 191, 197, 203, 212, 224, 228, 233, 237, 246, 252, 257, 267, 271, 275, 279, 284, 294, 309, 325,
        331, 337, 345, 362, 372, 380, 389, 394, 406, 412, 416, 423, 428, 440, 448, 454, 468, 475, 482, 493, 497, 506,
        513, 518, 524, 532, 540, 545, 550, 557, 566, 574, 578, 587, 599, 604, 614, 619, 624, 631, 637, 646, 656, 666,
        680, 695, 708, 716, 723, 729, 742, 750, 756, 762, 769, 774, 786, 792, 800, 809, 815, 825, 831, 838, 842, 847,
        852, 860, 866, 877, 889, 898, 902, 910, 918, 926, 936, 945, 948, 967, 975, 981, 986, 993, 1002, 1011, 1015,
        1034, 1044, 1054, 1063, 1084, 1097, 1105, 1110, 1120, 1125, 1129, 1137 };
static const uint8
        genre_tab[] =
                "BluesClassicRockCountryDanceDiscoFunkGrungeHip-HopJazz\
MetalNewAgeOldiesOtherPopR&BRapReggaeRockTechno\
IndustrialAlternativeSkaDeathMetalPranksSoundtrackEuro-TechnoAmbientTrip-HopVocal\
Jazz+FunkFusionTranceClassicalInstrumentalAcidHouseGameSoundClipGospel\
NoiseAlternRockBassSoulPunkSpaceMeditativeInstrumentalPopInstrumentalRockEthnic\
GothicDarkwaveTechno-IndustrialElectronicPop-FolkEurodanceDreamSouthernRockComedyCult\
GangstaTop40ChristianRapPop/FunkJungleNativeAmericanCabaretNewWavePsychadelicRave\
ShowtunesTrailerLo-FiTribalAcidPunkAcidJazzPolkaRetroMusicalRock&Roll\
HardRockFolkFolk-RockNationalFolkSwingFastFusionBebobLatinRevivalCeltic\
BluegrassAvantgardeGothicRockProgessiveRockPsychedelicRockSymphonicRockSlowRockBigBandChorusEasyListening\
AcousticHumourSpeechChansonOperaChamberMusicSonataSymphonyBootyBassPrimus\
PornGrooveSatireSlowJamClubTangoSambaFolkloreBalladPowerBalladRhythmicSoul\
FreestyleDuetPunkRockDrumSoloAcapellaEuro-HouseDanceHallGoaDrum&BassClub-HouseHardcore\
TerrorIndieBritPopNegerpunkPolskPunkBeatChristianGangstaRapHeavyMetalBlackMetalCrossover\
ContemporaryChristianChristianRockMerengueSalsaTrashMetalAnimeJPopSynthpop";

extern uint8 id3_temp_buf[SECTOR_SIZE];
extern uint8 key_buf[KEY_BUF_LEN];
extern uint8 check_count;
extern uint8 check_flag[8]; //查找ID3要素标记
extern uint16 id3_pos_buffer; //当前TempBuf未处理字符的索引
extern uint32 id3_pos_file; //当前文件的精确指针

extern id3_save_t *id3_save_p; //FrameID存储位置
extern id3_info_t *id3_info_p; //ap层提供的存储结构

/******************************************************************************/
/*
 * \par  Description: 查找风格字符串
 *
 * \param[in]    id3_info_r--id3信息
 genre_index--风格序号
 * \param[out]

 * \return       TRUE OR FALSE

 * \note
 *******************************************************************************/
bool mp3_select_genre(id3_info_t* id3_info_r, uint8 genre_index)
{
    uint8 *pSaveaddr;
    uint16 savelen;
    if (genre_index > 147)
    {
        id3_info_r->genre_buffer[0] = 0x00;
        return FALSE;
    }

    pSaveaddr = (uint8 *) (genre_tab + TabAddr[genre_index]);
    savelen = TabAddr[(genre_index + 1) % (sizeof(TabAddr) / 2)] - TabAddr[genre_index];
    if (savelen > (id3_info_r->genre_length - 1))
    {
        savelen = id3_info_r->genre_length - 1;
    }
    libc_memcpy(id3_info_r->genre_buffer, pSaveaddr, (uint32) savelen);
    id3_info_r->genre_buffer[savelen] = '\0'; //这里补字符串结束
    return TRUE;
}

/******************************************************************************/
/*
 * \par  Description:解析mp3类型的V1版本id3信息
 *
 * \param[in]

 * \param[out]

 * \return        TRUE OR FALSE

 * \note
 *******************************************************************************/
bool mp3_v1_parse(void)
{
    uint32 file_len;

    if (FALSE == vfs_file_get_size(vfs_mount, &file_len, id3_handle, 0))
    {
        return FALSE;
    }

    if (file_len < FILE_SIZE_MIN)
    {
        return FALSE;
    }

    vfs_file_seek(vfs_mount, file_len - 128, SEEK_SET, id3_handle);
    vfs_file_read(vfs_mount, id3_temp_buf, 128, id3_handle);

    if ((id3_temp_buf[0] == 'T') && (id3_temp_buf[1] == 'A') && (id3_temp_buf[2] == 'G'))
    {
        if (id3_info_p->tit2_length != 0)
        {
            libc_memcpy(id3_info_p->tit2_buffer, (char*) &id3_temp_buf[3], id3_info_p->tit2_length - 2);
            *((uint8 *) (id3_info_p->tit2_buffer) + id3_info_p->tit2_length - 2) = 0x00; //这里补字符串结束
        }

        if (id3_info_p->tpe1_length != 0)
        {
            libc_memcpy(id3_info_p->tpe1_buffer, (char*) &id3_temp_buf[33], id3_info_p->tpe1_length - 2);
            *((uint8 *) (id3_info_p->tpe1_buffer) + id3_info_p->tpe1_length - 2) = 0x00; //这里补字符串结束
        }

        if (id3_info_p->talb_length != 0)
        {
            libc_memcpy(id3_info_p->talb_buffer, (char*) &id3_temp_buf[63], id3_info_p->talb_length - 2);
            *((uint8 *) (id3_info_p->talb_buffer) + id3_info_p->talb_length - 2) = 0x00; //这里补字符串结束
        }

        if (id3_info_p->genre_length != 0)
        {
            mp3_select_genre(id3_info_p, id3_temp_buf[127]);
        }

        id3_info_p->track_num = (uint16) (id3_temp_buf[126]);

        return TRUE;
    }
    else
    {
        return FALSE;
    }
}
