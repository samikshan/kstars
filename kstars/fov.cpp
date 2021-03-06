/***************************************************************************
                          fov.cpp  -  description
                             -------------------
    begin                : Fri 05 Sept 2003
    copyright            : (C) 2003 by Jason Harris
    email                : kstars@30doradus.org
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#include "fov.h"
#include "Options.h"

#include <algorithm>

#include <qpainter.h>
#include <QTextStream>
#include <QFile>
#include <kdebug.h>
#include <kstandarddirs.h>

FOV::Shape FOV::intToShape(int s)
{ 
    return (s >= FOV::UNKNOWN || s < 0) ? FOV::UNKNOWN : static_cast<FOV::Shape>(s);
} 

FOV::FOV( const QString &n, float a, float b, Shape sh, const QString &col ) :
    m_name( n ), m_color( col ), m_sizeX( a ), m_shape( sh )
{ 
    m_sizeY = (b < 0.0) ? a : b;
} 

FOV::FOV() :
    m_name( i18n( "No FOV" ) ), m_color( "#FFFFFF" ), m_sizeX( 0.0 ), m_sizeY( 0.0 ), m_shape( SQUARE )
{}

void FOV::draw( QPainter &p, float zoomFactor ) {
    p.setPen( QColor( color() ) );
    p.setBrush( Qt::NoBrush );
    
    p.setRenderHint( QPainter::Antialiasing, Options::useAntialias() );

    float pixelSizeX = sizeX() * zoomFactor / 57.3 / 60.0;
    float pixelSizeY = sizeY() * zoomFactor / 57.3 / 60.0;
    QPointF center = p.viewport().center();
    switch ( shape() ) {
    case SQUARE: 
        p.drawRect( center.x() - pixelSizeX/2, center.y() - pixelSizeY/2, pixelSizeX, pixelSizeY);
        break;
    case CIRCLE: 
        p.drawEllipse( center, pixelSizeX/2, pixelSizeY/2 );
        break;
    case CROSSHAIRS: 
        //Draw radial lines
        p.drawLine(center.x() + 0.5*pixelSizeX, center.y(),
                   center.x() + 1.5*pixelSizeX, center.y());
        p.drawLine(center.x() - 0.5*pixelSizeX, center.y(),
                   center.x() - 1.5*pixelSizeX, center.y());
        p.drawLine(center.x(), center.y() + 0.5*pixelSizeY,
                   center.x(), center.y() + 1.5*pixelSizeY);
        p.drawLine(center.x(), center.y() - 0.5*pixelSizeY,
                   center.x(), center.y() - 1.5*pixelSizeY);
        //Draw circles at 0.5 & 1 degrees
        p.drawEllipse( center, 0.5 * pixelSizeX, 0.5 * pixelSizeY);
        p.drawEllipse( center,       pixelSizeX,       pixelSizeY);
        break;
    case BULLSEYE: 
        p.drawEllipse(center, 0.5 * pixelSizeX, 0.5 * pixelSizeY);
        p.drawEllipse(center, 2.0 * pixelSizeX, 2.0 * pixelSizeY);
        p.drawEllipse(center, 4.0 * pixelSizeX, 4.0 * pixelSizeY);
        break;
    case SOLIDCIRCLE: {
        QColor colorAlpha = color();
        colorAlpha.setAlpha(127);
        p.setBrush( QBrush( colorAlpha ) );
        p.drawEllipse(center, pixelSizeX/2, pixelSizeY/2 );
        p.setBrush(Qt::NoBrush);
        break;
    }
    default: ; 
    }
}

void FOV::draw(QPainter &p, float x, float y)
{
    float xfactor = x / sizeX() * 57.3 * 60.0;
    float yfactor = y / sizeY() * 57.3 * 60.0;
    float zoomFactor = std::min(xfactor, yfactor);
    switch( shape() ) {
    case CROSSHAIRS: zoomFactor /= 3; break;
    case BULLSEYE:   zoomFactor /= 8; break;
    default: ;
    }
    draw(p, zoomFactor);
}

void FOV::setShape( int s)
{
    m_shape = intToShape(s);
}


QList<FOV*> FOV::defaults()
{
    QList<FOV*> fovs;
    fovs << new FOV(i18nc("use field-of-view for binoculars", "7x35 Binoculars" ),
                    558,  558,  CIRCLE,"#AAAAAA")
         << new FOV(i18nc("use a Telrad field-of-view indicator", "Telrad" ),
                    30,   30,   BULLSEYE,"#AA0000")
         << new FOV(i18nc("use 1-degree field-of-view indicator", "One Degree"),
                    60,   60,   CIRCLE,"#AAAAAA")
         << new FOV(i18nc("use HST field-of-view indicator", "HST WFPC2"),
                    2.4,  2.4,  SQUARE,"#AAAAAA")
         << new FOV(i18nc("use Radiotelescope HPBW", "30m at 1.3cm" ),
                    1.79, 1.79, SQUARE,"#AAAAAA");
    return fovs;
}

void FOV::writeFOVs(const QList<FOV*> fovs)
{
    QFile f;
    f.setFileName( KStandardDirs::locateLocal( "appdata", "fov.dat" ) );

    if ( ! f.open( QIODevice::WriteOnly ) ) {
        kDebug() << i18n( "Could not open fov.dat." );
        return;
    }
    QTextStream ostream(&f);
    foreach(FOV* fov, fovs) {
        ostream << fov->name()  << ':'
                << fov->sizeX() << ':'
                << fov->sizeY() << ':'
                << QString::number( fov->shape() ) << ':' //FIXME: is this needed???
                << fov->color() << endl;
    }
    f.close();
}

QList<FOV*> FOV::readFOVs()
{
    QFile f;
    QList<FOV*> fovs;
    f.setFileName( KStandardDirs::locateLocal( "appdata", "fov.dat" ) );

    if( !f.exists() ) {
        fovs = defaults();
        writeFOVs(fovs);
        return fovs;
    }

    if( f.open(QIODevice::ReadOnly) ) {
        fovs.clear();
        QTextStream istream(&f);
        while( !istream.atEnd() ) {
            QStringList fields = istream.readLine().split(':');
            bool ok;
            QString name, color;
            float   sizeX, sizeY;
            Shape   shape;
            if( fields.count() == 4 ) {
                name = fields[0];
                sizeX = fields[1].toFloat(&ok);
                if( !ok ) {
                    return QList<FOV*>();
                }
                sizeY = sizeX;
                shape = intToShape( fields[2].toInt(&ok) );
                if( !ok ) {
                    return QList<FOV*>();
                }
                color = fields[3];
            } else if( fields.count() == 5 ) {
                name = fields[0];
                sizeX = fields[1].toFloat(&ok);
                if( !ok ) {
                    return QList<FOV*>();
                }
                sizeY = fields[2].toFloat(&ok);
                if( !ok ) {
                    return QList<FOV*>();
                }
                shape = intToShape( fields[3].toInt(&ok) );
                if( !ok ) {
                    return QList<FOV*>();
                }
                color = fields[4];
            } else {
                continue;
            }
            fovs.append( new FOV(name, sizeX, sizeY, shape, color) );
        }
    }
    return fovs;
}
