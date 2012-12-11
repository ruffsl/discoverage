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
    , m_showVectorField(false)
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

    setShowVectorField(config.value("show-vector-field",  false).toBool());

    config.endGroup();
    end();
}

void Config::save(QSettings& config)
{
    config.beginGroup("config");

    config.setValue("show-vector-field", showVectorField());

    config.endGroup();
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

// kate: replace-tabs on; indent-width 4;
