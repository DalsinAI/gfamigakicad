/* pcbnew without Python (AmigaOS has none yet): the scripting tool, the
 * action plugin menus and the script lists are there and empty. The board
 * helpers (LoadBoard, SaveBoard, CreateEmptyBoard) are upstream's own,
 * built without Python.h.
 *
 * Copyright (c) 2026 AmiCradle Team. GPL-3.0-or-later, as KiCad. */
#include <action_plugin.h>
#include <pcb_edit_frame.h>
#include <pcbnew_settings.h>
#include <settings/settings_manager.h>
#include <tool/action_menu.h>
#include <tool/action_toolbar.h>
#include <tools/pcb_actions.h>
#include <confirm.h>
#include <board.h>
#include <pcb_draw_panel_gal.h>
#include <tool/tool_manager.h>
#include <tools/pcb_selection_tool.h>
#include <gal/graphics_abstraction_layer.h>
#include <board_design_settings.h>
#include <view/view.h>
#include "pcb_scripting_tool.h"
#include "pcbnew_scripting.h"

SCRIPTING_TOOL::SCRIPTING_TOOL() : PCB_TOOL_BASE( "pcbnew.ScriptingTool" ) {}
SCRIPTING_TOOL::~SCRIPTING_TOOL() {}
void SCRIPTING_TOOL::Reset( RESET_REASON aReason ) {}
bool SCRIPTING_TOOL::Init() { return true; }
void SCRIPTING_TOOL::ReloadPlugins() {}
void SCRIPTING_TOOL::callLoadPlugins() {}

void SCRIPTING_TOOL::ShowPluginFolder()
{
    DisplayInfoMessage( nullptr, _( "Python plugins are not available on AmigaOS yet." ) );
}

int SCRIPTING_TOOL::reloadPlugins( const TOOL_EVENT& aEvent ) { return 0; }

int SCRIPTING_TOOL::showPluginFolder( const TOOL_EVENT& aEvent )
{
    ShowPluginFolder();
    return 0;
}

void SCRIPTING_TOOL::setTransitions()
{
    Go( &SCRIPTING_TOOL::reloadPlugins, ACTIONS::pluginsReload.MakeEvent() );
    Go( &SCRIPTING_TOOL::showPluginFolder, PCB_ACTIONS::pluginsShowFolder.MakeEvent() );
}

void pcbnewGetUnloadableScriptNames( wxString& aNames ) { aNames.clear(); }
void pcbnewGetScriptsSearchPaths( wxString& aNames ) { aNames.clear(); }
void pcbnewGetWizardsBackTrace( wxString& aNames ) { aNames.clear(); }

void PCB_EDIT_FRAME::buildActionPluginMenus( ACTION_MENU* aActionMenu ) {}
void PCB_EDIT_FRAME::addActionPluginTools( ACTION_TOOLBAR* aToolbar ) {}

std::vector<LEGACY_OR_API_PLUGIN> PCB_EDIT_FRAME::GetOrderedActionPlugins()
{
    return {};
}

bool PCB_EDIT_FRAME::GetActionPluginButtonVisible( const wxString& aPluginPath, bool aPluginDefault )
{
    if( PCBNEW_SETTINGS* cfg = GetAppSettings<PCBNEW_SETTINGS>( "pcbnew" ) )
    {
        for( const auto& [path, visible] : cfg->m_VisibleActionPlugins )
        {
            if( path == aPluginPath )
                return visible;
        }
    }

    return aPluginDefault;
}

/* upstream's, as the API plugins (and undo of a plugin run) use it */
void PCB_EDIT_FRAME::RebuildAndRefresh()
{
    // The list of existing items after running the action script
    const BOARD_ITEM_SET items = GetBoard()->GetItemSet();

    // Sync selection with items selection state
    SELECTION&          selection = GetCurrentSelection();
    PCB_SELECTION_TOOL* selTool = m_toolManager->GetTool<PCB_SELECTION_TOOL>();
    EDA_ITEMS           to_add;
    EDA_ITEMS           to_remove;

    for( BOARD_ITEM* item : items )
    {
        if( item->IsSelected() && !selection.Contains( item ) )
        {
            item->ClearSelected(); // temporarily
            to_add.push_back( item );
        }
    }

    for( EDA_ITEM* item : selection.GetItems() )
    {
        if( !item->IsSelected() && item->IsBOARD_ITEM() )
            to_remove.push_back( static_cast<BOARD_ITEM*>( item ) );
    }

    if( !to_add.empty() )
        selTool->AddItemsToSel( &to_add );

    if( !to_remove.empty() )
        selTool->RemoveItemsFromSel( &to_remove );

    m_pcb->BuildConnectivity();

    PCB_DRAW_PANEL_GAL* canvas = GetCanvas();

    canvas->GetView()->Clear();
    canvas->GetView()->InitPreview();
    canvas->GetGAL()->SetGridOrigin( VECTOR2D( m_pcb->GetDesignSettings().GetGridOrigin() ) );
    canvas->DisplayBoard( m_pcb );

    // allow tools to re-add their view items (selection previews, grids, etc.)
    if( m_toolManager )
        m_toolManager->ResetTools( TOOL_BASE::REDRAW );

    // reload the drawing-sheet
    SetPageSettings( m_pcb->GetPageSettings() );

    canvas->SyncLayersVisibility( m_pcb );

    canvas->Refresh();
}
