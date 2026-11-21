//=============================================================================
// Copyright (C) 2002 Radical Entertainment Ltd.  All rights reserved.
//
// File:        xboxmain.cpp
//
// Description: This file contains the main enrty point to the game.
//
// History:     + Based on xbox main and winmain from squidney
//
//=============================================================================

//========================================
// System Includes
//========================================
// Standard Library
#include <string.h>
// Foundation Tech
#include <raddebug.hpp>
#include <radobject.hpp>
// file reading before radtech
#include <stdio.h>
#include <SDL_main.h>

#ifdef __SWITCH__
#include <switch.h>
#include <unistd.h>
#endif

#ifdef RAD_VITA
#include <unistd.h>
#include <psp2/kernel/clib.h>
unsigned int sceLibcHeapSize = 16 * 1024 * 1024;
#endif

//========================================
// Project Includes
//========================================
#include <main/game.h>
#include <main/win32platform.h>
#include <main/singletons.h>
#include <main/commandlineoptions.h>
#include <memory/memoryutilities.h>
#include <memory/srrmemory.h>
#include <p3d/entity.hpp>

//========================================
// Forward Declarations
//========================================
static void ProcessCommandLineArguments( int argc, char *argv[] );

static void ProcessCommandLineArgumentsFromFile();

#ifdef SRR2_STARTUP_DIAGNOSTICS
static FILE* gStartupLog = NULL;

static void LogStartup( const char* stage, const char* detail = NULL )
{
    if( gStartupLog != NULL )
    {
        fprintf( gStartupLog, "%s%s%s\n", stage, detail != NULL ? ": " : "", detail != NULL ? detail : "" );
        fflush( gStartupLog );
    }
}
#endif

static void LogOutputFunction( void *userdata, int category, SDL_LogPriority priority, const char *message )
{
#ifdef RAD_VITA
    sceClibPrintf( "%s\n", message );
#else
    printf( "%s\n", message );
    fflush( stdout );
#endif
#ifdef SRR2_STARTUP_DIAGNOSTICS
    LogStartup( "SDL", message );
#endif
}

//=============================================================================
// Function:    WinMain
//=============================================================================
//
// Description: Main Windows entry point.
// 
// Parameters:  win32 parameters
//
// Returns:     win32 return.
//
//=============================================================================
extern "C" int main( int argc, char *argv[] )
{
#ifdef SRR2_STARTUP_DIAGNOSTICS
    gStartupLog = fopen( "sdmc:/switch/simpsons/srr2-startup.log", "w" );
    if( gStartupLog == NULL )
    {
        gStartupLog = fopen( "srr2-startup.log", "w" );
    }
    LogStartup( "main entered" );
#endif
#ifdef __SWITCH__
#ifdef RAD_DEBUG
    socketInitializeDefault();
    nxlinkStdio();
#endif
#ifdef SRR2_USE_PC_ASSETS
    if( chdir( "sdmc:/switch/simpsons" ) != 0 )
    {
        fprintf( stderr, "Unable to set game asset directory to sdmc:/switch/simpsons\n" );
#ifdef SRR2_STARTUP_DIAGNOSTICS
        LogStartup( "failed to change to game asset directory" );
        if( gStartupLog != NULL )
        {
            fclose( gStartupLog );
            gStartupLog = NULL;
        }
#endif
        return 1;
    }
#else
#ifdef SRR2_STARTUP_DIAGNOSTICS
    Result romfsResult = romfsInit();
    char romfsResultText[32];
    snprintf( romfsResultText, sizeof( romfsResultText ), "0x%08lx", (unsigned long)romfsResult );
    LogStartup( "romfsInit returned", romfsResultText );
#else
    romfsInit();
#endif
#endif
#endif
#ifdef RAD_VITA
	chdir( "ux0:data/simpsons" );
#endif

    //
    // Pick out and store command line settings.
    //
    CommandLineOptions::InitDefaults();
    ProcessCommandLineArguments( argc, argv );
    ProcessCommandLineArgumentsFromFile();

    //
    // Initialize SDL subsystems
    //
#if SDL_MAJOR_VERSION < 3
    int sdlInitResult = SDL_Init( SDL_INIT_EVENTS | SDL_INIT_VIDEO | SDL_INIT_GAMECONTROLLER );

    SDL_LogSetOutputFunction( LogOutputFunction, NULL );
#else
    int sdlInitResult = SDL_Init( SDL_INIT_EVENTS | SDL_INIT_VIDEO | SDL_INIT_GAMEPAD );

    SDL_SetLogOutputFunction( LogOutputFunction, NULL );
#endif
#ifdef SRR2_STARTUP_DIAGNOSTICS
    LogStartup( "SDL_Init returned", sdlInitResult == 0 ? "success" : SDL_GetError() );
#endif
    if( sdlInitResult != 0 )
    {
        fprintf( stderr, "SDL_Init failed: %s\n", SDL_GetError() );
#ifdef SRR2_STARTUP_DIAGNOSTICS
        if( gStartupLog != NULL )
        {
            fclose( gStartupLog );
            gStartupLog = NULL;
        }
#endif
        return 1;
    }

    //
    // Have to get FTech setup first so that we can use all the memory services.
    // The initialize window call will fail if another Simpsons window exists. In
    // this case, we exit.
    //
#ifdef SRR2_STARTUP_DIAGNOSTICS
    LogStartup( "creating window" );
#endif
    SDL_ClearError();
    if( !Win32Platform::InitializeWindow() )
    {
        const char* windowError = SDL_GetError();
        if( windowError != NULL && windowError[0] != '\0' )
        {
            fprintf( stderr, "Window initialization failed: %s\n", windowError );
#ifdef SRR2_STARTUP_DIAGNOSTICS
            LogStartup( "window initialization failed", windowError );
            if( gStartupLog != NULL )
            {
                fclose( gStartupLog );
                gStartupLog = NULL;
            }
#endif
            SDL_Quit();
            return 1;
        }
#ifdef SRR2_STARTUP_DIAGNOSTICS
        LogStartup( "window initialization returned false" );
        if( gStartupLog != NULL )
        {
            fclose( gStartupLog );
            gStartupLog = NULL;
        }
#endif
        return 0;
    }
#ifdef SRR2_STARTUP_DIAGNOSTICS
    LogStartup( "window created" );
#endif
    Win32Platform::InitializeFoundation();
#ifdef SRR2_STARTUP_DIAGNOSTICS
    LogStartup( "foundation initialized" );
#endif

    srand (Game::GetRandomSeed ());


    // Now disable the default heap
    //
#ifndef RAD_RELEASE
    tName::SetAllocator (GMA_DEBUG);

    //g_HeapActivityTracker.EnableHeapAllocs (GMA_DEFAULT, false);
    //g_HeapActivityTracker.EnableHeapFrees (GMA_DEFAULT, false);
#endif

    HeapMgr()->PushHeap (GMA_PERSISTENT);

    //
    // Instantiate all the singletons before doing anything else.
    //
    CreateSingletons();

    //
    // Construct the platform object.
    //
    Win32Platform* pPlatform = Win32Platform::CreateInstance();
    rAssert( pPlatform != NULL );

    //
    // Create the game object.
    //
    Game* pGame = Game::CreateInstance( pPlatform );
    rAssert( pGame != NULL );


    //
    // Initialize the game.
    //
#ifdef SRR2_STARTUP_DIAGNOSTICS
    LogStartup( "game initialization started" );
#endif
    pGame->Initialize();
#ifdef SRR2_STARTUP_DIAGNOSTICS
    LogStartup( "game initialization completed" );
#endif

    HeapMgr()->PopHeap (GMA_PERSISTENT);
#ifdef SRR2_STARTUP_DIAGNOSTICS
    LogStartup( "persistent heap popped" );
    LogStartup( "entering game loop" );
#endif

    //
    // Run it!  Control will not return from here until the game is stopped.
    //
    pGame->Run();
#ifdef SRR2_STARTUP_DIAGNOSTICS
    LogStartup( "game loop returned" );
#endif

    //
    // Terminate the game (this frees all resources allocated by the game).
    //
    pGame->Terminate();

    //
    // Dump all the singletons.
    //
    DestroySingletons();

    //
    // Destroy the game object.
    //
    Game::DestroyInstance();

    //
    // Shutdown the platform.
    //
    pPlatform->ShutdownPlatform();

    //
    // Destroy the game and platform (do it in this order in case the game's 
    // destructor references the platform.
    //
    Win32Platform::DestroyInstance();

    // As a last thing, shut down the memory.
    Win32Platform::ShutdownMemory();

    // Re-enable the default heap
    //
#ifndef RAD_RELEASE
    tName::SetAllocator (RADMEMORY_ALLOC_DEFAULT);
#endif

    //
    // Shutdown SDL subsystems
    //
    SDL_Quit();
#ifdef SRR2_STARTUP_DIAGNOSTICS
    LogStartup( "normal shutdown" );
    if( gStartupLog != NULL )
    {
        fclose( gStartupLog );
        gStartupLog = NULL;
    }
#endif

    //
    // Pass any error codes back to the operating system.
    //
    return 0;
}


//=============================================================================
// Function:    ProcessCommandLineArguments
//=============================================================================
//
// Description: Pick out the command line options and store them.
// 
// Parameters:  None.
//
// Returns:     None.
//
//=============================================================================
void ProcessCommandLineArguments(int argc, char* argv[])
{
    rDebugPrintf( "*************************************************************************\n" );
    rDebugPrintf( "Command Line Args:\n" );

    //
    // Pick out all the command line options and store them in GameDB.
    // Also dump them to the output for handy dandy viewing.
    //
    for (int i = 1; i < argc; i++ )
    {
        rDebugPrintf( "arg%d: %s\n", i, argv[i] );

        CommandLineOptions::HandleOption( argv[i] );
    }

    if( !CommandLineOptions::Get( CLO_ART_STATS ) )
    {
        //CommandLineOptions::HandleOption( "noheaps" );
    }

    rDebugPrintf( "*************************************************************************\n" );
}



void ProcessCommandLineArgumentsFromFile()
{
#ifndef FINAL

    //Chuck: looking for additional command line args being passed in from a file
    //its for QA testing etc.

    FILE* pfile = fopen( "command.txt", "r" );

    if (pfile != NULL)
    {
        int ret = fseek( pfile, 0, SEEK_END ); 
        rAssert( ret == 0 );

        int len = ftell( pfile );
        rAssertMsg( len < 256, "Command line file too large to process." );

        rewind( pfile );

        if( len > 0 && len < 256 )
        {
            char commandlinestring[256] = {0};

            fgets( commandlinestring, 256, pfile );
                    
            char* argument = strtok(commandlinestring," ");
            while (argument != NULL)
            {
                CommandLineOptions::HandleOption(argument);
                argument=strtok(NULL," ");
            }
        }

        fclose( pfile );
    }
#endif //FINAL
} //end of Function
