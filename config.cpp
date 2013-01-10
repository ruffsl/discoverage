/* This file is part of the DisCoverage project.

   Copyright (C) Dominik Haumann <dhaumann at rtr.tu-darmstadt.de>

   This program is free software; you can redistribute it and/or
   modify it under the terms of the GNU General Public License
   as published by the Free Software Foundation; either version 2
   of the License, or (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software Foundation,
   Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
*/

#include "config.h"

#include <QtCore/QSettings>

Config* Config::s_self = 0;

Config* Config::self()
{
    if (!s_self) {
        new Config();
    }

    return s_self;
}

Config::Config()
    : QObject()
    , m_refCount(0)
    , m_showPartition(false)
    , m_showDensity(false)
    , m_showVectorField(false)
    , m_showPreviewTrajectory(false)
    , m_zoomFactor(8.0)
{
    s_self = this;
}

Config::~Config()
{
    s_self = 0;
}

void Config::begin()
{
    ++m_refCount;
}

void Config::end()
{
    --m_refCount;

    if (m_refCount == 0) {
        emit configChanged();
    }
}


void Config::load(QSettings& config)
{
    begin();
    config.beginGroup("config");

    setShowPartition(config.value("show-partition",  false).toBool());
    setShowDensity(config.value("show-density",  false).toBool());
    setShowVectorField(config.value("show-vector-field",  false).toBool());
    setShowPreviewTrajectory(config.value("show-preview-trajectory",  false).toBool());
    m_zoomFactor = config.value("map-zoom-factor",  8.0).toDouble();

    config.endGroup();
    end();
}

void Config::save(QSettings& config)
{
    config.beginGroup("config");

    config.setValue("show-partition", showPartition());
    config.setValue("show-density", showDensity());
    config.setValue("show-vector-field", showVectorField());
    config.setValue("show-preview-trajectory", showPreviewTrajectory());
    config.setValue("map-zoom-factor", zoom());

    config.endGroup();
}

bool Config::showPartition() const
{
    return m_showPartition;
}

void Config::setShowPartition(bool show)
{
    if (m_showPartition == show)
        return;

    begin();
    m_showPartition = show;
    end();
}

bool Config::showDensity() const
{
    return m_showDensity;
}

void Config::setShowDensity(bool show)
{
    if (m_showDensity == show)
        return;

    begin();
    m_showDensity = show;
    end();
}

bool Config::showVectorField() const
{
    return m_showVectorField;
}

void Config::setShowVectorField(bool show)
{
    if (m_showVectorField == show)
        return;

    begin();
    m_showVectorField = show;
    end();
}

bool Config::showPreviewTrajectory() const
{
    return m_showPreviewTrajectory;
}

void Config::setShowPreviewTrajectory(bool show)
{
    if (m_showPreviewTrajectory == show)
        return;

    begin();
    m_showPreviewTrajectory = show;
    end();
}

bool Config::zoomIn()
{
    begin();
    m_zoomFactor += 1.0;
    end();

    return true;
}

bool Config::zoomOut()
{
    if (m_zoomFactor <= 1.0)
        return false;

    begin();
    m_zoomFactor -= 1.0;
    end();

    return true;
}

double Config::zoom()
{
    return m_zoomFactor;
}

// kate: replace-tabs on; indent-width 4;
