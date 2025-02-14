/*
 * screen_common.c
 *
 *  Created on: Jul 30, 2021
 *      Author: David
 */
#include "screen_common.h"

int32_t temp, dimTimer;
int16_t backupTemp, ambTemp;
uint8_t status, profile, Selected_Tip, lang, backupMode;
int8_t dimStep;
tipData_t backupTip;

char *tipName;
bool disableTipCopy;
bool newTip;
#ifdef ENABLE_DEBUG_SCREEN
bool dbg_scr_en;
#endif

plotData_t plot;

// Plot graph data update and drawing timer
void updatePlot(void){
  if(Iron.Error.Flags & _ACTIVE){
    return;
  }

  if(plot.timeStep<20){ plot.timeStep = 20; }
  if((current_time-plot.timer)>=plot.timeStep){                                          // Only store values if running
    plot.update=plot.enabled;
    plot.timer=current_time;
    plot.d[plot.index] = last_TIP_C;
    if(++plot.index>99){
      plot.index=0;
    }
  }
}

int longClickReturn(widget_t *w){
  selectable_widget_t *sel=NULL;
  if(w->type!=widget_combo){
    extractSelectablePartFromWidget(w);
  }
  else{
    comboBox_item_t *combo =  ((comboBox_widget_t*)w->content)->currentItem;
    if((combo->type == combo_Editable) || (combo->type == combo_MultiOption)){
      sel = &combo->widget->selectable;
    }
  }
  if(!sel || (sel && (sel->state!=widget_edit))){
      return screen_main;
  }
  return -1;
}

uint8_t update_GUI_Timer(void){
  static uint32_t guiTimer=0;
  if((current_time-guiTimer)>=systemSettings.settings.guiUpdateDelay){
    guiTimer=current_time;
    return 1;
  }
  return 0;
}

int autoReturn_ProcessInput(screen_t * scr, RE_Rotation_t input, RE_State_t *state){
  updatePlot();
  refreshOledDim();
  handleOledDim();
  if(input!=Rotate_Nothing){
    screen_timer=current_time;
  }
  if(input==Rotate_Decrement_while_click){
    if(scr==&Screen_settings){
      return screen_main;
    }
    else if(scr->current_widget->type==widget_combo){
      comboBox_item_t *item = ((comboBox_widget_t*)scr->current_widget->content)->currentItem;
      if(item->type==combo_Editable || item->type==combo_MultiOption){
        if(item->widget->selectable.state!=widget_edit){
          return screen_settings;
        }
      }
      else{
        return screen_settings;
      }
    }
  }
  if(input==LongClick){
    int x = longClickReturn(scr->current_widget);
    if (x!=-1){
      return x;
    }
  }

  if((current_time-screen_timer)>15000){
    return screen_main;
  }
  return default_screenProcessInput(scr, input, state);
}

void restore_contrast(void){
  if(getContrast() != systemSettings.settings.contrast){
    setContrast(systemSettings.settings.contrast);
  }
}

void refreshOledDim(void){
  dimTimer = current_time;
  if(dimStep<5 && getContrast()<systemSettings.settings.contrast ){
    if(getOledPower()==disable){
      setOledPower(enable);
    }
    dimStep=5;
  }
}

void handleOledDim(void){
  static uint32_t stepTimer;
  uint8_t contrast=getContrast();
  if(getCurrentMode()>mode_sleep && !getOledPower()){                   // If screen turned off and not in sleep mode, wake it.
    refreshOledDim();                                                   //(Something woke the station from sleep)
  }
  if(dimStep==0){
    if(systemSettings.settings.dim_mode && contrast>5 && ((current_time-dimTimer)>=systemSettings.settings.dim_Timeout)){
      dimStep=-5;
    }
    if(systemSettings.settings.dim_inSleep==disable && getCurrentMode()==mode_sleep && contrast==1 && (last_TIP_C<100 || (Iron.Error.Flags & _ACTIVE))){  // Turn of screen if temp<100ºC or error active
      setOledPower(disable);
    }
  }
  // Smooth screen brightness dimming
  else if(current_time-stepTimer>19){
    stepTimer = current_time;
    contrast+=dimStep;
    if(contrast>4 && (contrast<systemSettings.settings.contrast)){
      dimTimer=current_time;
      setContrast(contrast);
    }
    else{
      if(dimStep>0){
        restore_contrast();
      }
      else{
        setContrast(1);
      }
      dimTimer = current_time;
      dimStep=0;
    }
  }
}

void updateAmbientTemp(void){
  ambTemp = (last_NTC_C+5)/10;
}
