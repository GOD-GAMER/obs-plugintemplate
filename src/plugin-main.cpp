/*
 * OBS Audio Calibrator Plugin
 * Copyright (C) 2025
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

#include <obs-module.h>
#include <obs-frontend-api.h>
#include <plugin-support.h>
#include "calibration-dialog.hpp"

OBS_DECLARE_MODULE()
OBS_MODULE_USE_DEFAULT_LOCALE(PLUGIN_NAME, "en-US")

static CalibrationDialog *calibrationDialog = nullptr;

static void showCalibrationWizard()
{
    if (!calibrationDialog) {
        QWidget *parent = static_cast<QWidget*>(obs_frontend_get_main_window());
        calibrationDialog = new CalibrationDialog(parent);
        calibrationDialog->setAttribute(Qt::WA_DeleteOnClose);
        
        QObject::connect(calibrationDialog, &QDialog::destroyed, []() {
            calibrationDialog = nullptr;
        });
    }
    
    calibrationDialog->show();
    calibrationDialog->raise();
    calibrationDialog->activateWindow();
}

static void menuCallback(void *)
{
    showCalibrationWizard();
}

bool obs_module_load(void)
{
    obs_log(LOG_INFO, "OBS Audio Calibrator plugin loaded (version %s)", PLUGIN_VERSION);
    
    // Add menu item to Tools menu
    obs_frontend_add_tools_menu_item(
        "Audio Calibration Wizard",
        menuCallback,
        nullptr
    );
    
    obs_log(LOG_INFO, "Added 'Audio Calibration Wizard' to Tools menu");
    
    return true;
}

void obs_module_unload(void)
{
    if (calibrationDialog) {
        calibrationDialog->close();
        calibrationDialog = nullptr;
    }
    
    obs_log(LOG_INFO, "OBS Audio Calibrator plugin unloaded");
}

const char *obs_module_description(void)
{
    return "Audio Calibration Wizard - Automatically configure audio filters "
           "for consistent output levels based on your voice calibration.";
}
