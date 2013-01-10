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

#ifndef DISCOVERAGE_CONFIG_H
#define DISCOVERAGE_CONFIG_H

#include <QtCore/QObject>

class QSettings;

class Config : public QObject
{
    Q_OBJECT

    static Config* s_self;

    public:
        Config();
        virtual ~Config();

        static Config* self();

        void load(QSettings& config);
        void save(QSettings& config);

    public slots:
        void begin();
        void end();

    signals:
        void configChanged();

    //
    // config setters and getters
    //
    public slots:
        bool showPartition() const;
        void setShowPartition(bool show);

        bool showDensity() const;
        void setShowDensity(bool show);

        bool showVectorField() const;
        void setShowVectorField(bool show);

        bool showPreviewTrajectory() const;
        void setShowPreviewTrajectory(bool show);

        bool zoomIn();
        bool zoomOut();
        double zoom();

    private:
        int m_refCount;

        bool m_showPartition;
        bool m_showDensity;
        bool m_showVectorField;
        bool m_showPreviewTrajectory;
        double m_zoomFactor;
};

#endif // DISCOVERAGE_CONFIG_H

// kate: replace-tabs on; indent-width 4;
