/***
*
*       Copyright (c) 1996-2002, Valve LLC. All rights reserved.
*       
*       This product contains software technology licensed from Id 
*       Software, Inc. ("Id Technology").  Id Technology (c) 1996 Id Software, Inc. 
*       All Rights Reserved.
*
*   Use, distribution, and modification of this source code and/or resulting
*   object code is restricted to non-commercial enhancements to products from
*   Valve LLC.  All other use, distribution, or modification is prohibited
*   without written permission from Valve LLC.
*
****/
//
// death notice
//
#include "hud.h"
#include "cl_util.h"
#include "parsemsg.h"

#include <string.h>
#include <stdio.h>
#include "draw_util.h"

float color[3];

struct DeathNoticeItem {
        char szKiller[MAX_PLAYER_NAME_LENGTH*2];
        char szVictim[MAX_PLAYER_NAME_LENGTH*2];
        int iId;        // the index number of the associated sprite
        bool bSuicide;
        bool bTeamKill;
        bool bNonPlayerKill;
        float flDisplayTime;
        float *KillerColor;
        float *VictimColor;
        int iHeadShotId;
        bool bIsLocalPlayerKiller;
        bool bIsLocalPlayerVictim;
};

#define MAX_DEATHNOTICES        4
static int DEATHNOTICE_DISPLAY_TIME = 6;

#define DEATHNOTICE_TOP         32

DeathNoticeItem rgDeathNoticeList[ MAX_DEATHNOTICES + 1 ];

int CHudDeathNotice :: Init( void )
{
        gHUD.AddHudElem( this );

        HOOK_MESSAGE( gHUD.m_DeathNotice, DeathMsg );

        hud_deathnotice_time = CVAR_CREATE( "hud_deathnotice_time", "5", FCVAR_ARCHIVE );
        m_iFlags = 0;

        return 1;
}


void CHudDeathNotice :: InitHUDData( void )
{
        memset( rgDeathNoticeList, 0, sizeof(rgDeathNoticeList) );
}


int CHudDeathNotice :: VidInit( void )
{
        m_HUD_d_skull = gHUD.GetSpriteIndex( "d_skull" );
        m_HUD_d_headshot = gHUD.GetSpriteIndex("d_headshot");

        return 1;
}

int CHudDeathNotice :: Draw( float flTime )
{
        int x, y, box_x, box_y, box_w, box_h, i;
        int padding_x = 8; // Left/Right
        int padding_y = 6; // Top/Bottom
        int spacing_between = 18; // Space Between Messages
        int base_height = 16; 

        for( i = 0; i < MAX_DEATHNOTICES; i++ )
        {
                if ( rgDeathNoticeList[i].iId == 0 )
                        break;

                if ( rgDeathNoticeList[i].flDisplayTime < flTime )
                {
                        memmove( &rgDeathNoticeList[i], &rgDeathNoticeList[i+1], sizeof(DeathNoticeItem) * (MAX_DEATHNOTICES - i) );
                        i--;
                        continue;
                }

                rgDeathNoticeList[i].flDisplayTime = min( rgDeathNoticeList[i].flDisplayTime, flTime + DEATHNOTICE_DISPLAY_TIME );

                float fade_time = rgDeathNoticeList[i].flDisplayTime - flTime;
                float alpha_multiplier = 1.0f;
                
                if ( fade_time < 1.0f )
                        alpha_multiplier = fade_time;

                if( !g_iUser1 )
                {
                        y = YRES(DEATHNOTICE_TOP) + 2 + ((base_height + spacing_between) * i);
                }
                else
                {
                        y = ScreenHeight / 5 + 2 + ((base_height + spacing_between) * i);
                }

                int id = (rgDeathNoticeList[i].iId == -1) ? m_HUD_d_skull : rgDeathNoticeList[i].iId;
                int sprite_width = gHUD.GetSpriteRect(id).Width();
                int sprite_height = gHUD.GetSpriteRect(id).Height();
                int headshot_width = rgDeathNoticeList[i].iHeadShotId ? gHUD.GetSpriteRect(m_HUD_d_headshot).Width() : 0;

                int killer_text_width = 0;
                int victim_text_width = DrawUtils::ConsoleStringLen(rgDeathNoticeList[i].szVictim);
                
                if ( !rgDeathNoticeList[i].bSuicide )
                        killer_text_width = DrawUtils::ConsoleStringLen( rgDeathNoticeList[i].szKiller );

                box_w = padding_x + killer_text_width + padding_x + sprite_width + headshot_width + victim_text_width + padding_x + padding_x;
                box_h = base_height;

                box_x = ScreenWidth - box_w - 23; // Left
                box_y = y - padding_y;

                int bg_r = 0, bg_g = 0, bg_b = 0, bg_a = 100;
                int stroke_r = 0, stroke_g = 0, stroke_b = 0;
                bool draw_stroke = false;
                
                if ( rgDeathNoticeList[i].bIsLocalPlayerVictim )
                {
                        bg_r = 150;
                        bg_g = 10;
                        bg_b = 10;
                        bg_a = int(120 * alpha_multiplier);
                        draw_stroke = false;
                }
                else if ( rgDeathNoticeList[i].bIsLocalPlayerKiller )
                {
                        bg_r = 20;
                        bg_g = 20;
                        bg_b = 20;
                        bg_a = int(80 * alpha_multiplier);
                        stroke_r = 255;
                        stroke_g = 0;
                        stroke_b = 0;
                        draw_stroke = true;
                }
                else
                {
                        bg_r = 20;
                        bg_g = 20;
                        bg_b = 20;
                        bg_a = int(70 * alpha_multiplier);
                        draw_stroke = false;
                }

                DrawUtils::DrawRectangleExt( box_x, box_y, box_w, box_h + (padding_y * 2),
                                            bg_r, bg_g, bg_b, bg_a,
                                            stroke_r, stroke_g, stroke_b, draw_stroke, 2 );

                x = box_x + padding_x;
                y = box_y + padding_y;

                if ( !rgDeathNoticeList[i].bSuicide )
                {
                        if ( rgDeathNoticeList[i].KillerColor )
                                DrawUtils::SetConsoleTextColor( rgDeathNoticeList[i].KillerColor[0], rgDeathNoticeList[i].KillerColor[1], rgDeathNoticeList[i].KillerColor[2] );
                        x = DrawUtils::DrawConsoleString( x, y, rgDeathNoticeList[i].szKiller );
                        x += padding_x;
                }

                SPR_Set( gHUD.GetSprite(id), 255, 255, 255 );
                SPR_DrawAdditive( 0, x, y, &gHUD.GetSpriteRect(id) );
                x += sprite_width;

                if( rgDeathNoticeList[i].iHeadShotId)
                {
                        SPR_Set( gHUD.GetSprite(m_HUD_d_headshot), 255, 255, 255 );
                        SPR_DrawAdditive( 0, x, y, &gHUD.GetSpriteRect(m_HUD_d_headshot));
                        x += headshot_width;
                }

                if (!rgDeathNoticeList[i].bNonPlayerKill)
                {
                        if ( rgDeathNoticeList[i].VictimColor )
                                DrawUtils::SetConsoleTextColor( rgDeathNoticeList[i].VictimColor[0], rgDeathNoticeList[i].VictimColor[1], rgDeathNoticeList[i].VictimColor[2] );
                        x += padding_x;
                        x = DrawUtils::DrawConsoleString( x, y, rgDeathNoticeList[i].szVictim );
                }
        }

        if( i == 0 )
                m_iFlags &= ~HUD_DRAW;

        return 1;
}

// This message handler may be better off elsewhere
int CHudDeathNotice :: MsgFunc_DeathMsg( const char *pszName, int iSize, void *pbuf )
{
        m_iFlags |= HUD_DRAW;

        BufferReader reader( pszName, pbuf, iSize );

        int killer = reader.ReadByte();
        int victim = reader.ReadByte();
        int headshot = reader.ReadByte();

        char killedwith[32];
        strncpy( killedwith, "d_", sizeof(killedwith) );
        strncat( killedwith, reader.ReadString(), sizeof( killedwith ) - 2 );

        //if (gViewPort)
        //      gViewPort->DeathMsg( killer, victim );
        gHUD.m_Scoreboard.DeathMsg( killer, victim );

        gHUD.m_Spectator.DeathMessage(victim);
        int i;
        for ( i = 0; i < MAX_DEATHNOTICES; i++ )
        {
                if ( rgDeathNoticeList[i].iId == 0 )
                        break;
        }
        if ( i == MAX_DEATHNOTICES )
        { // move the rest of the list forward to make room for this item
                memmove( rgDeathNoticeList, rgDeathNoticeList+1, sizeof(DeathNoticeItem) * MAX_DEATHNOTICES );
                i = MAX_DEATHNOTICES - 1;
        }

        //if (gViewPort)
                //gViewPort->GetAllPlayersInfo();
        gHUD.m_Scoreboard.GetAllPlayersInfo();

        // Get the Killer's name
        const char *killer_name = g_PlayerInfoList[ killer ].name;
        if ( !killer_name )
        {
                killer_name = "";
                rgDeathNoticeList[i].szKiller[0] = 0;
        }
        else
        {
                rgDeathNoticeList[i].KillerColor = GetClientColor( killer );
                strncpy( rgDeathNoticeList[i].szKiller, killer_name, MAX_PLAYER_NAME_LENGTH );
                rgDeathNoticeList[i].szKiller[MAX_PLAYER_NAME_LENGTH-1] = 0;
                rgDeathNoticeList[i].bIsLocalPlayerKiller = ( g_PlayerInfoList[ killer ].thisplayer != 0 );
        }

        // Get the Victim's name
        const char *victim_name = NULL;
        // If victim is -1, the killer killed a specific, non-player object (like a sentrygun)
        if ( victim != -1 )
                victim_name = g_PlayerInfoList[ victim ].name;
        if ( !victim_name )
        {
                victim_name = "";
                rgDeathNoticeList[i].szVictim[0] = 0;
        }
        else
        {
                rgDeathNoticeList[i].VictimColor = GetClientColor( victim );
                strncpy( rgDeathNoticeList[i].szVictim, victim_name, MAX_PLAYER_NAME_LENGTH );
                rgDeathNoticeList[i].szVictim[MAX_PLAYER_NAME_LENGTH-1] = 0;
                rgDeathNoticeList[i].bIsLocalPlayerVictim = ( g_PlayerInfoList[ victim ].thisplayer != 0 );
        }

        // Is it a non-player object kill?
        if ( victim == -1 )
        {
                rgDeathNoticeList[i].bNonPlayerKill = true;

                // Store the object's name in the Victim slot (skip the d_ bit)
                strncpy( rgDeathNoticeList[i].szVictim, killedwith+2, sizeof(killedwith) );
        }
        else
        {
                if ( killer == victim || killer == 0 )
                        rgDeathNoticeList[i].bSuicide = true;

                if ( !strncmp( killedwith, "d_teammate", sizeof(killedwith)  ) )
                        rgDeathNoticeList[i].bTeamKill = true;
        }

        rgDeathNoticeList[i].iHeadShotId = headshot;

        // Find the sprite in the list
        int spr = gHUD.GetSpriteIndex( killedwith );

        rgDeathNoticeList[i].iId = spr;

        rgDeathNoticeList[i].flDisplayTime = gHUD.m_flTime + hud_deathnotice_time->value;


        if (rgDeathNoticeList[i].bNonPlayerKill)
        {
                ConsolePrint( rgDeathNoticeList[i].szKiller );
                ConsolePrint( " killed a " );
                ConsolePrint( rgDeathNoticeList[i].szVictim );
                ConsolePrint( "\n" );
        }
        else
        {
                // record the death notice in the console
                if ( rgDeathNoticeList[i].bSuicide )
                {
                        ConsolePrint( rgDeathNoticeList[i].szVictim );

                        if ( !strncmp( killedwith, "d_world", sizeof(killedwith)  ) )
                        {
                                ConsolePrint( " died" );
                        }
                        else
                        {
                                ConsolePrint( " killed self" );
                        }
                }
                else if ( rgDeathNoticeList[i].bTeamKill )
                {
                        ConsolePrint( rgDeathNoticeList[i].szKiller );
                        ConsolePrint( " killed his teammate " );
                        ConsolePrint( rgDeathNoticeList[i].szVictim );
                }
                else
                {
                        if( headshot )
                                ConsolePrint( "*** ");
                        ConsolePrint( rgDeathNoticeList[i].szKiller );
                        ConsolePrint( " killed " );
                        ConsolePrint( rgDeathNoticeList[i].szVictim );
                }

                if ( *killedwith && (*killedwith > 13 ) && strncmp( killedwith, "d_world", sizeof(killedwith) ) && !rgDeathNoticeList[i].bTeamKill )
                {
                        if ( headshot )
                                ConsolePrint(" with a headshot from ");
                        else
                                ConsolePrint(" with ");

                        ConsolePrint( killedwith+2 ); // skip over the "d_" part
                }

                if( headshot ) ConsolePrint( " ***");
                ConsolePrint( "\n" );
        }

        return 1;
}