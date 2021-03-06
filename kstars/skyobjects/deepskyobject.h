/***************************************************************************
                          deepskyobject.h  -  K Desktop Planetarium
                             -------------------
    begin                : Sun Feb 11 2001
    copyright            : (C) 2001 by Jason Harris
    email                : jharris@30doradus.org
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#ifndef DEEPSKYOBJECT_H_
#define DEEPSKYOBJECT_H_

#include <qimage.h>
#include <qpoint.h>

#include "skyobject.h"
#include "dms.h"

class QImage;
class QString;
class KSPopupMenu;
class CustomCatalogComponent;

/**
  *@class DeepSkyObject
	*Provides all necessary information about a deep-sky object:
	*data inherited from SkyObject (coordinates, type, magnitude, 
	*2 names, and URLs) and data specific to DeepSkyObjects
	*(common name, angular size, position angle, Image, catalog)
	*@short Information about a "dep-sky" object; i.e., anything 
	*that's not a solar system body or a star.
	*@author Jason Harris
	*@version 1.0
	*/
class DeepSkyObject : public SkyObject {
public:
    /**
      *Constructor.  Create DeepSkyObject with data according to arguments.
    	*@param t Type of object
    	*@param r catalog Right Ascension
    	*@param d catalog Declination
    	*@param m magnitude (brightness)
    	*@param n Primary name
    	*@param n2 Secondary name
    	*@param lname Long name (common name)
    	*@param cat catalog ID
    	*@param a major axis (arcminutes)
    	*@param b minor axis (arcminutes)
    	*@param pa position angle (degrees)
    	*@param pgc PGC catalog number
    	*@param ugc UGC catalog number
    	*/
    explicit DeepSkyObject( int t=SkyObject::STAR,
                            dms r=dms(0.0), dms d=dms(0.0), float m=0.0,
                            const QString &n="unnamed", const QString &n2=QString(),
                            const QString &lname=QString(), const QString &cat=QString(),
                            float a=0.0, float b=0.0, double pa=0.0,
                            int pgc=0, int ugc=0 );

    /**
      *Constructor.  Create DeepSkyObject with data according to arguments.  
    	*Differs from above function only in data type of RA and Dec.
    	*@param t Type of object
    	*@param r catalog Right Ascension
    	*@param d catalog Declination
    	*@param m magnitude (brightness)
    	*@param n Primary name
    	*@param n2 Secondary name
    	*@param lname Long name (common name)
    	*@param cat catalog ID
    	*@param a major axis (arcminutes)
    	*@param b minor axis (arcminutes)
    	*@param pa position angle (degrees)
    	*@param pgc PGC catalog number
    	*@param ugc UGC catalog number
    	*/
    DeepSkyObject( int t, double r, double d, float m=0.0,
                   const QString &n="unnamed", const QString &n2=QString(),
                   const QString &lname=QString(), const QString &cat=QString(),
                   float a=0.0, float b=0.0, double pa=0.0, int pgc=0, int ugc=0 );

    /** @short Copy constructor.
     *  @param o SkyObject from which to copy data
     */
    DeepSkyObject(const DeepSkyObject &o );

    QString labelString() const;

    virtual DeepSkyObject* clone() const;
    virtual SkyObject::UID getUID() const;
    
    /** *Destructor */
    virtual ~DeepSkyObject() { }

    /**
      *@enum CATALOG
    	*The catalog ID of the DeepSkyObject.
    	*/
    enum CATALOG { CAT_MESSIER=0, CAT_NGC=1, CAT_IC=2, CAT_UNKNOWN };

    /**
      *@return a QString identifying the object's primary catalog.
    	*Possible catalog values are:
    	*- "M" for Messier catalog
    	*- "NGC" for NGC catalog
    	*- "IC" for IC catalog
    	*- empty string is presumed to be an object in a custom catalog
    	*@sa setCatalog()
    	*/
    QString catalog( void ) const;

    /**
      *Set the internal Catalog value according to the QString
    	*argument:
    	* "M"   : CAT_MESSIER
    	* "NGC" : CAT_NGC
    	* "IC"  : CAT_IC
    	*@sa catalog()
    	*/
    void setCatalog( const QString &s );

    /**
      *Set the the refrence to the custom catalog component, if any
      *@sa customCatalog()
      */
    inline void setCustomCatalog(CustomCatalogComponent *s) { customCat = s; }

    /**
      *@return a pointer to a custom catalog component
    */
    inline CustomCatalogComponent * customCatalog() { return customCat; }

    /**
      *Set the integrated flux value of the object
      */
    inline void setFlux(const float &f) { Flux = f; }

    /**
      *@return the object's integrated flux, unit value is stored in the custom catalog component.
        */
    inline float flux() const { return Flux; }

    /**
      *@return the object's major axis length, in arcminutes.
    	*/
    inline float a() const { return MajorAxis; }

    /**
      *@return the object's minor axis length, in arcminutes.
    	*/
    inline float b() const { return MinorAxis; }

    /**
      *@return the object's aspect ratio (MinorAxis/MajorAxis).  Returns 1.0
    	*if the object's MinorAxis=0.0.
    	*/
    float e() const;

    /**
      *@return the object's position angle, meausred clockwise from North.
    	*/
    inline virtual double pa() const { return PositionAngle; }

    /**
      *@return the object's UGC catalog number.  Return 0 if the object is not in UGC.
    	*/
    inline int ugc() const { return UGC; }

    /**
      *@return the object's PGC catalog number.  Return 0 if the object is not in PGC.
    	*/
    inline int pgc() const { return PGC; }

    /** @return an object's image */
    const QImage& image() const { return m_image; }

    /** Try to load the object's image */
    void loadImage();

    /**
      *@return true if the object is in the Messier catalog
    	*/
    inline bool isCatalogM() const { return (Catalog == CAT_MESSIER); }

    /**
      *@return true if the object is in the NGC catalog
    	*/
    inline bool isCatalogNGC() const { return (Catalog == CAT_NGC); }

    /**
      *@return true if the object is in the IC catalog
    	*/
    inline bool isCatalogIC() const { return (Catalog == CAT_IC); }

    /**
      *@return true if the object is not in a catalog
    	*/
    inline bool isCatalogNone() const { return (Catalog == CAT_UNKNOWN); }

    /**
    	*@return the pixel distance for offseting the object's name label
    	*/
    virtual double labelOffset() const;

    quint64 updateID;
    quint64 updateNumID;

private:
    virtual void initPopupMenu( KSPopupMenu *pmenu );

    unsigned char Catalog;
    double PositionAngle;
    int UGC, PGC;
    float MajorAxis, MinorAxis, Flux;
    QImage m_image;
    CustomCatalogComponent *customCat;
};

#endif
