/***************************************************************************
                          solarsystemsinglecomponent.cpp  -  K Desktop Planetarium
                             -------------------
    begin                : 2005/30/08
    copyright            : (C) 2005 by Thomas Kabelmann
    email                : thomas.kabelmann@gmx.de
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#include <QPainter>

#include "skycomponent.h"
#include "solarsystemsinglecomponent.h"
#include "solarsystemcomposite.h"

#include "dms.h"
#include "kstars.h"
#include "kstarsdata.h"
#include "ksplanetbase.h"
#include "ksplanet.h"
#include "skymap.h"
#include "Options.h"

SolarSystemSingleComponent::SolarSystemSingleComponent(SolarSystemComposite *parent, KSPlanetBase *kspb, bool (*visibleMethod)(), int msize)
: SingleComponent( parent, visibleMethod )
{
	minsize = msize;
	sizeScale = 1.0;
	setStoredObject( kspb );
	m_Earth = parent->earth();
}

SolarSystemSingleComponent::~SolarSystemSingleComponent()
{
	//Object deletes handled by parent class (SingleComponent)
}

void SolarSystemSingleComponent::init(KStarsData *data) {
	ksp()->loadData();

	if ( ! ksp()->name().isEmpty() ) parent()->objectNames().append( ksp()->name() );
	if ( ! ksp()->longname().isEmpty() && ksp()->longname() != ksp()->name() )
		parent()->objectNames().append( ksp()->longname() );
}

void SolarSystemSingleComponent::updatePlanets(KStarsData *data, KSNumbers *num) {
	if ( visible() ) {
		ksp()->findPosition( num, data->geo()->lat(), data->lst(), earth() );
		ksp()->EquatorialToHorizontal( data->lst(), data->geo()->lat() );

		if ( ksp()->hasTrail() ) 
			ksp()->updateTrail( data->lst(), data->geo()->lat() );
	}
}

bool SolarSystemSingleComponent::addTrail( SkyObject *o ) {
	if ( o == skyObject() ) {
		ksp()->addToTrail();
		return true;
	}
	return false;
}

bool SolarSystemSingleComponent::hasTrail( SkyObject *o, bool *found ) {
	if ( o == skyObject() ) {
		*found = true;
		return ksp()->hasTrail();
	}
	return false;
}

bool SolarSystemSingleComponent::removeTrail( SkyObject *o ) {
	if ( o == skyObject() ) {
		ksp()->clearTrail();
		return true;
	}
	return false;
}

void SolarSystemSingleComponent::draw( KStars *ks, QPainter &psky, double scale ) {
	if ( !visible() ) return;

	SkyMap *map = ks->map();

	//TODO: default values for 2nd & 3rd arg. of SkyMap::checkVisibility()
	if ( map->checkVisibility( ksp() ) ) {
		float Width = scale * map->width();
		float Height = scale * map->height();

		float sizemin = 4.0;
		if ( ksp()->name() == "Sun" || ksp()->name() == "Moon" ) sizemin = 8.0;
		sizemin = sizemin * scale;

		//TODO: KSPlanetBase needs a color property, and someone has to set the planet's color
		psky.setPen( ksp()->color() );
		psky.setBrush( ksp()->color() );
		QPointF o = map->getXY( ksp(), Options::useAltAz(), Options::useRefraction(), scale );

		//Is planet onscreen?
		if ( o.x() >= 0. && o.x() <= Width && o.y() >= 0. && o.y() <= Height ) {
			float size = ksp()->angSize() * scale * dms::PI * Options::zoomFactor()/10800.0;
			if ( size < sizemin ) size = sizemin;

			//Draw planet image if:
			if ( Options::showPlanetImages() &&  //user wants them,
//FIXME:					int(Options::zoomFactor()) >= int(zoommin) &&  //zoomed in enough,
					! ksp()->image()->isNull() &&  //image loaded ok,
					size < Width ) {  //and size isn't too big.

				//Image size must be modified to account for possibility that rotated image's
				//size is bigger than original image size.  The rotated image is a square
				//superscribed on the original image.  The superscribed square is larger than
				//the original square by a factor of (cos(t) + sin(t)) where t is the angle by
				//which the two squares are rotated (in our case, equal to the position angle +
				//the north angle, reduced between 0 and 90 degrees).
				//The proof is left as an exercise to the student :)
				dms pa( map->findPA( ksp(), o.x(), o.y(), scale ) );
				double spa, cpa;
				pa.SinCos( spa, cpa );
				cpa = fabs(cpa);
				spa = fabs(spa);
				size = size * (cpa + spa);

				//Because Saturn has rings, we inflate its image size by a factor 2.5
				if ( ksp()->name() == "Saturn" ) size = int(2.5*size);

//FIXME: resize_mult ??
//				if (resize_mult != 1) {
//					size *= resize_mult;
//				}

				ksp()->scaleRotateImage( size, pa.Degrees() );
				float x1 = o.x() - 0.5*ksp()->image()->width();
				float y1 = o.y() - 0.5*ksp()->image()->height();
				psky.drawImage( QPointF(x1, y1), *( ksp()->image() ) );

			} else { //Otherwise, draw a simple circle.

				psky.drawEllipse( QRectF(o.x()-0.5*size, o.y()-0.5*size, size, size) );
			}

			//draw Name
			if ( Options::showPlanetNames() ) {
				psky.setPen( QColor( ks->data()->colorScheme()->colorNamed( "PNameColor" ) ) );
				drawNameLabel( psky, ksp(), o.x(), o.y(), scale );
			}
		}
	}
}

void SolarSystemSingleComponent::drawTrails( KStars *ks, QPainter& psky, double scale ) {
	if ( ! visible() || ! ksp()->hasTrail() ) return;
	
	SkyMap *map = ks->map();
	KStarsData *data = ks->data();

	float Width = scale * map->width();
	float Height = scale * map->height();

	QColor tcolor1 = QColor( data->colorScheme()->colorNamed( "PlanetTrailColor" ) );
	QColor tcolor2 = QColor( data->colorScheme()->colorNamed( "SkyColor" ) );

	SkyPoint p = ksp()->trail().first();
	QPointF o = map->getXY( &p, Options::useAltAz(), Options::useRefraction(), scale );
	QPointF oLast( o );

	bool doDrawLine(false);
	int i = 0;
	int n = ksp()->trail().size();

	if ( ( o.x() >= -1000. && o.x() <= Width+1000. 
			&& o.y() >= -1000. && o.y() <= Height+1000. ) ) {
//		psky.moveTo(o.x(), o.y());
		doDrawLine = true;
	}

	psky.setPen( QPen( tcolor1, 1 ) );
	bool firstPoint( true );
	foreach ( p, ksp()->trail() ) {
		if ( firstPoint ) { firstPoint = false; continue; } //skip first point

		if ( Options::fadePlanetTrails() ) {
			//Define interpolated color
			QColor tcolor = QColor(
						(i*tcolor1.red()   + (n-i)*tcolor2.red())/n,
						(i*tcolor1.green() + (n-i)*tcolor2.green())/n,
						(i*tcolor1.blue()  + (n-i)*tcolor2.blue())/n );
			++i;
			psky.setPen( QPen( tcolor, 1 ) );
		}

		o = map->getXY( &p, Options::useAltAz(), Options::useRefraction(), scale );
		if ( ( o.x() >= -1000 && o.x() <= Width+1000 && o.y() >=-1000 && o.y() <= Height+1000 ) ) {

			//Want to disable line-drawing if this point and the last are both outside bounds of display.
			//FIXME: map->rect() should return QRectF
			if ( ! map->rect().contains( o.toPoint() ) && ! map->rect().contains( oLast.toPoint() ) ) doDrawLine = false;

			if ( doDrawLine ) {
//				psky.lineTo( o.x(), o.y() );
				psky.drawLine( oLast, o );
			} else {
//				psky.moveTo( o.x(), o.y() );
				doDrawLine = true;
			}
		}

		oLast = o;
	}
}

// see SkyComponent::drawNameLabel()
float SolarSystemSingleComponent::labelSize(SkyObject *obj, double scale)
{
	float size = ksp()->angSize() * scale * dms::PI * Options::zoomFactor()/10800.0;
// TODO init following components with these minsize values
// 	COMET minsize = 2;
// 	Sun = 8;
	if (size < minsize)
	{
		size = minsize;
	}
	// saturn is scaled by factor 2.5
	size = sizeScale * size;
	return size;
}


void SolarSystemSingleComponent::setSizeScale(float scale)
{
	sizeScale = scale;
}