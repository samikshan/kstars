/***************************************************************************
                          skyobject.h  -  K Desktop Planetarium
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

#ifndef SKYOBJECT_H_
#define SKYOBJECT_H_

#include <QString>
#include <QStringList>
#include <QSharedDataPointer>

#include <klocale.h>

#include "skypoint.h"
#include "dms.h"
#include "auxinfo.h"

class QPoint;
class QPainter;
class GeoLocation;
class KStarsDateTime;
class KSPopupMenu;

/**
 *@class SkyObject
 *Provides all necessary information about an object in the sky:
 *its coordinates, name(s), type, magnitude, and QStringLists of
 *URLs for images and webpages regarding the object.
 *@short Information about an object in the sky.
 *@author Jason Harris
 *@version 1.0
 */
class SkyObject : public SkyPoint {
public:
    /** @short Type for Unique object IDenticator.
     *
     * Each object has unique ID (UID). For different objects UIDs
     * must be different.
     */
    typedef qint64 UID;

    /** @short Kind of UID */
    static const UID UID_STAR;
    static const UID UID_GALAXY;
    static const UID UID_DEEPSKY;
    static const UID UID_SOLARSYS;
    
    /** Invalid UID. Real sky object could not have such UID */
    static const UID invalidUID;
    
    /**
     *Constructor.  Set SkyObject data according to arguments.
     *@param t Type of object
     *@param r catalog Right Ascension
     *@param d catalog Declination
     *@param m magnitude (brightness)
     *@param n Primary name
     *@param n2 Secondary name
     *@param lname Long name (common name)
     */
    explicit SkyObject( int t=TYPE_UNKNOWN, dms r=dms(0.0), dms d=dms(0.0),
                        float m=0.0, const QString &n=QString(), const QString &n2=QString(), const QString &lname=QString() );
    /**
     *Constructor.  Set SkyObject data according to arguments.  Differs from
     *above function only in data type of RA and Dec.
     *@param t Type of object
     *@param r catalog Right Ascension
     *@param d catalog Declination
     *@param m magnitude (brightness)
     *@param n Primary name
     *@param n2 Secondary name
     *@param lname Long name (common name)
     */
    SkyObject( int t, double r, double d, float m=0.0,
               const QString &n=QString(), const QString &n2=QString(), const QString &lname=QString() );

    /** Destructor (empty) */
    virtual ~SkyObject();

    /** Create copy of object.
     * This method is virtual copy constructor. It allows for safe
     * copying of objects. In other words, KSPlanet object stored in
     * SkyObject pointer will be copied as KSPlanet.
     * Each subclass of SkyObject MUST implement clone method.
     *
     *  @return pointer to newly allocated object. Caller takes full responsibility
     *  for deallocating it. 
     */
    virtual SkyObject* clone() const;
    /**
     *@enum TYPE
     *The type classification of the SkyObject.
     */
    enum TYPE { STAR=0, CATALOG_STAR=1, PLANET=2, OPEN_CLUSTER=3, GLOBULAR_CLUSTER=4,
                GASEOUS_NEBULA=5, PLANETARY_NEBULA=6, SUPERNOVA_REMNANT=7, GALAXY=8,
                COMET=9, ASTEROID=10, CONSTELLATION=11, MOON=12, ASTERISM=13, 
                GALAXY_CLUSTER=14, DARK_NEBULA=15, QUASAR=16, MULT_STAR=17, RADIO_SOURCE=18,
                SATELLITE=19, TYPE_UNKNOWN };

    /**
     *@return object's primary name.
     */
    inline virtual QString name( void ) const { return hasName() ? Name : unnamedString;}

    /**
     *@return object's primary name, translated to local language.
     */
    inline QString translatedName() const { return i18n( name().toUtf8() );}

    /** @return object's secondary name */
    inline QString name2( void ) const { return ( hasName2() ? Name2 : emptyString ); }

    /** @return object's secondary name, translated to local language. */
    inline QString translatedName2() const { return ( hasName2() ? i18n( Name2.toUtf8() ): emptyString );}

    /**
     *@return object's common (long) name
     */
    virtual QString longname( void ) const { return hasLongName() ? LongName : unnamedObjectString; }

    /**
     *@return object's common (long) name, translated to local language.
     */
    QString translatedLongName() const { return i18n( longname().toUtf8() );}

    /**
     *Set the object's long name.
     *@param longname the object's long name.
     */
    void setLongName( const QString &longname=QString() );

    /**
     *@return the string used to label the object on the map
     *In the default implementation, this just returns translatedName()
     *Overridden by StarObject.
     */
    virtual QString labelString() const;

    /**
     *@return object's type identifier (int)
     *@see enum TYPE
     */
    inline int type( void ) const { return (int)Type; }

    /**
     *Set the object's type identifier to the argument.
     *@param t the object's type identifier (e.g., "SkyObject::PLANETARY_NEBULA")
     *@see enum TYPE
     */
    inline void setType( int t ) { Type = (unsigned char)t; }

    /** *@return a string describing object's type. */
    QString typeName() const;

    /**
     *@return object's magnitude
     */
    inline float mag( void ) const { return Magnitude; }

    /**
     *@return the object's position angle.  This is overridden in KSPlanetBase
     *and DeepSkyObject; for all other SkyObjects, this returns 0.0.
     */
    inline virtual double pa() const { return 0.0; }

    /** *@return true if the object is a solar system body. */
    inline bool isSolarSystem() const { return ( type() == 2 || type() == 9 || type() == 10 || type() == 12 ); }

    /** Show Type-specific popup menu. Oveloading is done in the function initPopupMenu */
    void showPopupMenu( KSPopupMenu *pmenu, const QPoint &pos );

    /**
     *Determine the time at which the point will rise or set.  Because solar system
     *objects move across the sky, it is necessary to iterate on the solution.
     *We compute the rise/set time for the object's current position, then
     *compute the object's position at that time.  Finally, we recompute then
     *rise/set time for the new coordinates.  Further iteration is not necessary,
     *even for the most swiftly-moving object (the Moon).
     *@return the local time that the object will rise
     *@param dt current UT date/time
     *@param geo current geographic location
     *@param rst If true, compute rise time. If false, compute set time.
     *@param exact If true, use a second iteration for more accurate time
     */
    QTime riseSetTime( const KStarsDateTime &dt, const GeoLocation *geo, bool rst, bool exact=true );

    /**
     *@return the UT time when the object will rise or set
     *@param dt  target date/time
     *@param geo pointer to Geographic location
     *@param rst Boolean. If true will compute rise time. If false
     *       will compute set time.
     *@param exact If true, use a second iteration for more accurate time
     */
    QTime riseSetTimeUT( const KStarsDateTime &dt, const GeoLocation *geo, bool rst, bool exact=true );

    /**
     *@return the Azimuth time when the object will rise or set. This function
     *recomputes set or rise UT times.
     *@param dt  target date/time
     *@param geo GeoLocation object
     *@param rst Boolen. If true will compute rise time. If false
     *       will compute set time.
     */
    dms riseSetTimeAz( const KStarsDateTime &dt, const GeoLocation *geo, bool rst);

    /**
     *The same iteration technique described in riseSetTime() is used here.
     *@return the local time that the object will transit the meridian.
     *@param dt  target date/time
     *@param geo pointer to the geographic location
     */
    QTime transitTime( const KStarsDateTime &dt, const GeoLocation *geo );

    /**
     *@return the universal time that the object will transit the meridian.
     *@param dt   target date/time
     *@param geo pointer to the geographic location
     */
    QTime transitTimeUT( const KStarsDateTime &dt, const GeoLocation *geo );

    /**
     *@return the altitude of the object at the moment it transits the meridian.
     *@param dt  target date/time
     *@param geo pointer to the geographic location
     */
    dms transitAltitude( const KStarsDateTime &dt, const GeoLocation *geo );

    /**
     *The coordinates for the object on date dt are computed and returned,
     *but the object's internal coordinates are not permanently modified.
     *@return the coordinates of the selected object for the time given by jd
     *@param dt  date/time for which the coords will be computed.
     *@param geo pointer to geographic location (used for solar system only)
     */
    SkyPoint recomputeCoords( const KStarsDateTime &dt, const GeoLocation *geo=0 );

    inline bool hasName() const { return ! Name.isEmpty(); }

    inline bool hasName2() const { return ! Name2.isEmpty(); }

    inline bool hasLongName() const { return ! LongName.isEmpty(); }

    /**
     *@short Given the Image title from a URL file, try to convert it to an image credit string.
     */
    QString messageFromTitle( const QString &imageTitle );

    /**
     *@short Save new user log text
      */
    void saveUserLog( const QString &newLog );

    /**
     *@return the pixel distance for offseting the object's name label
     *@note overridden in StarObject, DeepSkyObject, KSPlanetBase
     */
    virtual double labelOffset() const;

    /**
     *@short Query whether this SkyObject has a valid AuxInfo structure associated with it.
     *@return true if this SkyObject has a valid AuxInfo structure associated with it, false if not.
     */
    inline bool hasAuxInfo() { return ! (!info); }

    /**
     *@return a reference to a QStringList storing a list of Image URLs associated with this SkyObject
     */
    inline QStringList &ImageList() { return getAuxInfo()->ImageList; }

    /**
     *@return a reference to a QStringList storing a list of Image Titles associated with this SkyObject
     */
    inline QStringList &ImageTitle() { return getAuxInfo()->ImageTitle; }

    /**
     *@return a reference to a QStringList storing a list of Information Links associated with this SkyObject
     */
    inline QStringList &InfoList() { return getAuxInfo()->InfoList; }

    /**
     *@return a reference to a QStringList storing a list of Information Link Titles associated with this SkyObject
     */
    inline QStringList &InfoTitle() { return getAuxInfo()->InfoTitle; }

    /**
     *@return a reference to a QString storing the users' log for this SkyObject
     */
    inline QString &userLog() { return getAuxInfo()->userLog; }

    inline QString &notes() { return getAuxInfo()->notes; }

    void setNotes( QString _notes) { getAuxInfo()->notes = _notes; }

    /** @short Return UID for object.  
     * This method should be reimplemented in all concrete
     * subclasses. Implementation for SkyObject just returns
     * invalidUID. It's required SkyObject is not an abstract class.
     */
    virtual UID getUID() const;

private:
    /** Initialize the popup menut. This function should call correct
     * initialization function in KSPopupMenu. By overloading the
     * function, we don't have to check the object type when we need
     * the menu. */
    virtual void initPopupMenu(KSPopupMenu* pmenu);

    /**
     *Compute the UT time when the object will rise or set. It is an auxiliary
     *procedure because it does not use the RA and DEC of the object but values
     *given as parameters. You may want to use riseSetTimeUT() which is
     *public.  riseSetTimeUT() calls this function iteratively.
     *@param dt     target date/time
     *@param geo    pointer to Geographic location
     *@param righta pointer to Right ascention of the object
     *@param decl   pointer to Declination of the object
     *@param rst    Boolean. If true will compute rise time. If false
     *              will compute set time.
     *@return the time at which the given position will rise or set.
     */
    QTime auxRiseSetTimeUT( const KStarsDateTime &dt, const GeoLocation *geo,
                            const dms *righta, const dms *decl, bool riseT);

    /**
     *Compute the LST time when the object will rise or set. It is an auxiliary
     *procedure because it does not use the RA and DEC of the object but values
     *given as parameters. You may want to use riseSetTimeLST() which is
     *public.  riseSetTimeLST() calls this function iteratively.
     *@param gLt Geographic latitude
     *@param rga Right ascention of the object
     *@param decl Declination of the object
     *@param rst Boolean. If true will compute rise time. If false
     *       will compute set time.
     */
    dms auxRiseSetTimeLST( const dms *gLt, const dms *rga, const dms *decl, bool rst );

    /**
     *Compute the approximate hour angle that an object with declination d will have
     *when its altitude is h (as seen from geographic latitude gLat).
     *This function is only used by auxRiseSetTimeLST().
     *@param h pointer to the altitude of the object
     *@param gLat pointer to the geographic latitude
     *@param d pointer to the declination of the object.
     *@return the Hour Angle, in degrees.
     */
    double approxHourAngle( const dms *h, const dms *gLat, const dms *d );

    /**
     *Correct for the geometric altitude of the center of the body at the
     *time of rising or setting. This is due to refraction at the horizon
     *and to the size of the body. The moon correction has also to take into
     *account parallax. The value we use here is a rough approximation
     *suggeted by J. Meeus.
     *
     *Weather status (temperature and pressure basically) is not taken
     *into account although change of conditions between summer and 
     *winter could shift the times of sunrise and sunset by 20 seconds.
     *
     *This function is only used by auxRiseSetTimeLST().
     *@return dms object with the correction.
     */
    dms elevationCorrection(void);

    /**
     *@short Return a pointer to the AuxInfo object associated with this SkyObject.
     *@note  This method creates the AuxInfo object if it is non-existent
     *@return Pointer to an AuxInfo structure
     */
    AuxInfo *getAuxInfo();

    unsigned char Type;
    float Magnitude;

protected:
    /**Set the object's magnitude.
     * @param m the object's magnitude. */
    inline void setMag( float m ) { Magnitude = m < 30.0 ? m : 99.0 ; }

    /**Set the object's primary name.
     * @param name the object's primary name */
    inline void setName( const QString &name ) { Name = name; }

    /**Set the object's secondary name.
     * @param name2 the object's secondary name. */
    inline void setName2( const QString &name2=QString() ) { Name2 = name2; }

    QString Name, Name2, LongName;

    // Pointer to an auxiliary info structure that stores Image URLs, Info URLs etc.
    QSharedDataPointer<AuxInfo> info;

    // store often used name strings in static variables
    static QString emptyString;
    static QString unnamedString;
    static QString unnamedObjectString;
    static QString starString;
};

#endif
