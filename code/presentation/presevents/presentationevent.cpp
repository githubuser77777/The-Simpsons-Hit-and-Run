//=============================================================================
// Copyright (C) 2002 Radical Entertainment Ltd.  All rights reserved.
//
// File:        presentationevent.cpp
//
// Description: Implement PresentationEvent
//
// History:     22/05/2002 + Created -- NAME
//
//=============================================================================

//========================================
// System Includes
//========================================

// Foundation Tech
#include <raddebug.hpp>
#ifdef SRR2_STARTUP_DIAGNOSTICS
#include <SDL.h>
#endif

//========================================
// Project Includes
//========================================

#include <presentation/animplayer.h>
#include <presentation/presevents/presentationevent.h>

//******************************************************************************
//
// Global Data, Local Data, Local Classes
//
//******************************************************************************

//******************************************************************************
//
// Public Member Functions
//
//******************************************************************************

//==============================================================================
// PresentationEvent::PresentationEvent
//==============================================================================
// Description: Constructor.
//
// Parameters: None.
//
// Return:      N/A.
//
//==============================================================================
PresentationEvent::PresentationEvent()
{
    Init();
}

//==============================================================================
// PresentationEvent::~PresentationEvent
//==============================================================================
// Description: Destructor.
//
// Parameters: None.
//
// Return:      N/A.
//
//==============================================================================
PresentationEvent::~PresentationEvent()
{
    
}


//==============================================================================
// PresentationEvent::OnLoadDataComplete
//==============================================================================
//
// Description: 
//
// Parameters:  
//
// Return:      
//
//==============================================================================
void PresentationEvent::OnLoadDataComplete()
{
    if( pCallback != NULL )
    {
        pCallback->OnPresentationEventLoadComplete( this );
    }
}

//=============================================================================
// PresentationEvent::Update
//=============================================================================
// Description: Comment
//
// Parameters:  ( unsigned int elapsedTime )
//
// Return:      bool 
//
//=============================================================================
bool PresentationEvent::Update( unsigned int elapsedTime )
{
    AnimationPlayer* player = GetPlayer();
    
    player->Update( elapsedTime );

    return( player->IsPlaying() );
}

//=============================================================================
// PresentationEvent::Start
//=============================================================================
// Description: Comment
//
// Parameters:  ()
//
// Return:      void 
//
//=============================================================================
void PresentationEvent::Start()
{
    AnimationPlayer* player = GetPlayer();

#ifdef SRR2_STARTUP_DIAGNOSTICS
    SDL_Log( "SRR2 movie: PresentationEvent::Start file=%s loaded=%d", fileName, mbLoaded );
#endif
    if( !mbLoaded )
    {
        player->LoadData( fileName, this, false, GetUserData() );

        mbLoaded = true;
#ifdef SRR2_STARTUP_DIAGNOSTICS
        SDL_Log( "SRR2 movie: PresentationEvent::LoadData returned file=%s", fileName );
#endif
    }
    player->SetKeepLayersFrozen( mbKeepLayersFrozen );
	player->SetSkippable(mbIsSkippable);
    player->Play();
#ifdef SRR2_STARTUP_DIAGNOSTICS
    SDL_Log( "SRR2 movie: PresentationEvent::Play returned file=%s", fileName );
#endif
}

//=============================================================================
// PresentationEvent::Stop
//=============================================================================
// Description: Comment
//
// Parameters:  ()
//
// Return:      void 
//
//=============================================================================
void PresentationEvent::Stop()
{
    AnimationPlayer* player = GetPlayer();

#ifdef SRR2_STARTUP_DIAGNOSTICS
    SDL_Log( "SRR2 movie: PresentationEvent::Stop file=%s clear=%d", fileName, mbClearWhenDone );
#endif
    if( mbClearWhenDone )
    {
        player->ClearData();
    }
    else
    {
        player->Reset();
    }
#ifdef SRR2_STARTUP_DIAGNOSTICS
    SDL_Log( "SRR2 movie: PresentationEvent::Stop completed file=%s", fileName );
#endif
}

//=============================================================================
// PresentationEvent::Init
//=============================================================================
// Description: Comment
//
// Parameters:  ()
//
// Return:      void 
//
//=============================================================================
void PresentationEvent::Init()
{
    mbAutoPlay = true;
    mbClearWhenDone = true;
    mbLoaded = false;
    mbKeepLayersFrozen = false;
	mbIsSkippable = true;
}

//******************************************************************************
//
// Private Member Functions
//
//******************************************************************************

