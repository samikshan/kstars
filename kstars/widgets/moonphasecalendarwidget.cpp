/***************************************************************************
             moonphasecalendarwidget.cpp  -  K Desktop Planetarium
                             -------------------
    begin                : Sat Jun 26 2010
    copyright            : (C) 2010 by Akarsh Simha
    email                : akarshsimha@gmail.com
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#include "moonphasecalendarwidget.h"

#include "skyobjects/ksplanet.h"
#include "ksnumbers.h"
#include "kstarsdatetime.h"
#include "ksutils.h"

#include <kcolorscheme.h>
#include <kglobal.h>
#include <kglobalsettings.h>
#include <kdebug.h>
#include <kcalendarsystem.h>

#include <QActionEvent>
#include <QPainter>
#include <QStyle>
#include <QtGui/QStyleOptionViewItem>
#include <QFile>

#include <cmath>

MoonPhaseCalendar::MoonPhaseCalendar( KSMoon &moon, QWidget *parent ) : KDateTable(parent), m_Moon(moon) {
    
    // Populate moon images from disk into the hash
    numDayColumns = calendar()->daysInWeek( QDate::currentDate() );
    numWeekRows = 7;
    imagesLoaded = false;

    for( int i = 0; i < 36; ++i )
        m_Images[i] = NULL;

    // TODO: Set geometry.

}

MoonPhaseCalendar::~MoonPhaseCalendar() {
    // Free up the moon image pixmaps
    if( imagesLoaded ) {
        for( int i = 0; i < 36; ++i )
            delete m_Images[i];
    }
}

QSize MoonPhaseCalendar::sizeHint() const {
    const int suggestedMoonImageSize = 50;
    return QSize( qRound( ( suggestedMoonImageSize + 2 ) * numDayColumns ),
                  ( qRound( suggestedMoonImageSize + 4 + 12 ) * numWeekRows ) ); // FIXME: Using hard-coded fontsize
}

void MoonPhaseCalendar::loadImages() {
    computeMoonImageSize();
    kDebug() << "Loading moon images. MoonImageSize = " << MoonImageSize;
    for( int i = 0; i < 36; ++i ) {
        QString imName = QString().sprintf("moon%02d.png", i);
        QFile imFile;
        if( m_Images[i] )
            delete m_Images[i];
        m_Images[i] = NULL;
        if( KSUtils::openDataFile( imFile, imName ) ) {
            imFile.close();
            m_Images[i] = new QPixmap( QPixmap( imFile.fileName() ).scaled( MoonImageSize, MoonImageSize, Qt::KeepAspectRatio, Qt::SmoothTransformation ) );
        }
    }
    imagesLoaded = true;
}

void MoonPhaseCalendar::computeMoonImageSize() {
    cellWidth = width() / ( double ) numDayColumns;
    cellHeight = height() / ( double ) numWeekRows;
    kDebug() << cellWidth << cellHeight;
    MoonImageSize = ( (cellWidth > cellHeight - 12) ? cellHeight - 12 : cellWidth ) - 2; // FIXME: Using hard-coded fontsize
}

void MoonPhaseCalendar::setGeometry( int x, int y, int w, int h ) {
    imagesLoaded = false;
}

void MoonPhaseCalendar::setGeometry( const QRect &r ) {
    setGeometry( r.x(), r.y(), r.width(), r.height() ); // FIXME: +1 / -1 pixel compensation. Not required at the moment.
}


void MoonPhaseCalendar::paintEvent( QPaintEvent *e )
{
    QPainter p( this );
    if( !imagesLoaded )
        loadImages();
    KColorScheme colorScheme(palette().currentColorGroup(), KColorScheme::View);
    const QRect &rectToUpdate = e->rect();
    int leftCol = ( int )std::floor( rectToUpdate.left() / cellWidth );
    int topRow = ( int )std::floor( rectToUpdate.top() / cellHeight );
    int rightCol = ( int )std::ceil( rectToUpdate.right() / cellWidth );
    int bottomRow = ( int )std::ceil( rectToUpdate.bottom() / cellHeight );
    bottomRow = qMin( bottomRow, numWeekRows - 1 );
    rightCol = qMin( rightCol, numDayColumns - 1 );
    p.translate( leftCol * cellWidth, topRow * cellHeight );
    for ( int i = leftCol; i <= rightCol; ++i ) {
        for ( int j = topRow; j <= bottomRow; ++j ) {
            this->paintCell( &p, j, i, colorScheme );
            p.translate( 0, cellHeight );
        }
        p.translate( cellWidth, 0 );
        p.translate( 0, -cellHeight * ( bottomRow - topRow + 1 ) );
    }
    p.end();
}


void MoonPhaseCalendar::paintCell( QPainter *painter, int row, int col, const KColorScheme &colorScheme )
{
    double w = cellWidth - 1;
    double h = cellHeight - 1;
    QRectF cell = QRectF( 0, 0, w, h );
    QString cellText;
    QPen pen;
    QColor cellBackgroundColor, cellTextColor;
    QFont cellFont = KGlobalSettings::generalFont();
    bool workingDay = false;
    int cellWeekDay, pos;
    BackgroundMode cellBackgroundMode = RectangleMode;

    //   kDebug() << "In paintCell";

    //Calculate the position of the cell in the grid
    pos = numDayColumns * ( row - 1 ) + col;

    //Calculate what day of the week the cell is
    if ( col + calendar()->weekStartDay() <= numDayColumns ) {
        cellWeekDay = col + calendar()->weekStartDay();
    } else {
        cellWeekDay = col + calendar()->weekStartDay() - numDayColumns;
    }

    //See if cell day is normally a working day
    if ( KGlobal::locale()->workingWeekStartDay() <= KGlobal::locale()->workingWeekEndDay() ) {
        if ( cellWeekDay >= KGlobal::locale()->workingWeekStartDay() &&
             cellWeekDay <= KGlobal::locale()->workingWeekEndDay() ) {
                workingDay = true;
        }
    } else {
        if ( cellWeekDay >= KGlobal::locale()->workingWeekStartDay() ||
             cellWeekDay <= KGlobal::locale()->workingWeekEndDay() ) {
                workingDay = true;
        }
    }

    if( row == 0 ) {

        //We are drawing a header cell

        //If not a normal working day, then use "do not work today" color
        if ( workingDay ) {
            cellTextColor = palette().color(QPalette::WindowText);
        } else {
            KColorScheme colorScheme(palette().currentColorGroup(), KColorScheme::Window);
            cellTextColor = colorScheme.foreground(KColorScheme::NegativeText).color();
        }
        cellBackgroundColor = palette().color(QPalette::Window);

        //Set the text to the short day name and bold it
        cellFont.setBold( true );
        cellText = calendar()->weekDayName( cellWeekDay, KCalendarSystem::ShortDayName );

    } else {

        //We are drawing a day cell

        //Calculate the date the cell represents
        QDate cellDate = dateFromPos( pos );

        bool validDay = calendar()->isValid( cellDate );

        // Draw the day number in the cell, if the date is not valid then we don't want to show it
        if ( validDay ) {
            cellText = calendar()->dayString( cellDate, KCalendarSystem::ShortFormat );
        } else {
            cellText = "";
        }

        if( ! validDay || calendar()->month( cellDate ) != calendar()->month( date() ) ) {
            // we are either
            // ° painting an invalid day
            // ° painting a day of the previous month or
            // ° painting a day of the following month or
            cellBackgroundColor = palette().color(backgroundRole());
            cellTextColor = colorScheme.foreground(KColorScheme::InactiveText).color();
        } else {
            //Paint a day of the current month

            // Background Colour priorities will be (high-to-low):
            // * Selected Day Background Colour
            // * Customized Day Background Colour
            // * Normal Day Background Colour

            // Background Shape priorities will be (high-to-low):
            // * Customized Day Shape
            // * Normal Day Shape

            // Text Colour priorities will be (high-to-low):
            // * Customized Day Colour
            // * Day of Pray Colour (Red letter)
            // * Selected Day Colour
            // * Normal Day Colour

            //Determine various characteristics of the cell date
            bool selectedDay = ( cellDate == date() );
            bool currentDay = ( cellDate == QDate::currentDate() );
            bool dayOfPray = ( calendar()->dayOfWeek( cellDate ) == KGlobal::locale()->weekDayOfPray() );

            //Default values for a normal cell
            cellBackgroundColor = palette().color( backgroundRole() );
            cellTextColor = palette().color( foregroundRole() );

            // If we are drawing the current date, then draw it bold and active
            if ( currentDay ) {
                cellFont.setBold( true );
                cellTextColor = colorScheme.foreground(KColorScheme::ActiveText).color();
            }

            // if we are drawing the day cell currently selected in the table
            if ( selectedDay ) {
                // set the background to highlighted
                cellBackgroundColor = palette().color( QPalette::Highlight );
                cellTextColor = palette().color( QPalette::HighlightedText );
            }

            //If the cell day is the day of religious observance, then always color text red unless Custom overrides
            if ( dayOfPray ) {
                KColorScheme colorScheme(palette().currentColorGroup(),
                                         selectedDay ? KColorScheme::Selection : KColorScheme::View);
                cellTextColor = colorScheme.foreground(KColorScheme::NegativeText).color();
            }

        }
    }

    //Draw the background
    if (row == 0) {
        painter->setPen( cellBackgroundColor );
        painter->setBrush( cellBackgroundColor );
        painter->drawRect( cell );
    } else if (cellBackgroundColor != palette().color(backgroundRole())) {
        QStyleOptionViewItemV4 opt;
        opt.initFrom(this);
        opt.rect = cell.toRect();
        if (cellBackgroundColor != palette().color(backgroundRole())) {
            opt.palette.setBrush(QPalette::Highlight, cellBackgroundColor);
            opt.state |= QStyle::State_Selected;
        }
        if (false && opt.state & QStyle::State_Enabled) {
            opt.state |= QStyle::State_MouseOver;
        } else {
            opt.state &= ~QStyle::State_MouseOver;
        }
        opt.showDecorationSelected = true;
        opt.viewItemPosition = QStyleOptionViewItemV4::OnlyOne;
        style()->drawPrimitive(QStyle::PE_PanelItemViewItem, &opt, painter, this);
    }

    if( row != 0 ) {
        // Paint the moon phase
        QDate cellDate = dateFromPos( pos );
        if( calendar()->isValid( cellDate ) ) {
            int iPhase = computeMoonPhase( KStarsDateTime( cellDate, QTime(0, 0, 0) ) );
            QRect drawRect = cell.toRect();
            painter->drawPixmap( ( drawRect.width() - MoonImageSize )/2, 12 + (( drawRect.height() - 12 ) - MoonImageSize)/2, *m_Images[ iPhase ] ); // FIXME: Using hard coded fontsize
            //            kDebug() << "Drew moon image " << iPhase;
        }
    }

    //Draw the text
    painter->setPen( cellTextColor );
    painter->setFont( cellFont );
    painter->drawText( cell, (row == 0) ? Qt::AlignCenter : (Qt::AlignTop | Qt::AlignHCenter), cellText, &cell );

    //Draw the base line
    if (row == 0) {
        painter->setPen( palette().color(foregroundRole()) );
        painter->drawLine( QPointF( 0, h ), QPointF( w, h ) );
    }

    // If the day cell we just drew is bigger than the current max cell sizes,
    // then adjust the max to the current cell

    /*
    if ( cell.width() > d->maxCell.width() ) d->maxCell.setWidth( cell.width() );
    if ( cell.height() > d->maxCell.height() ) d->maxCell.setHeight( cell.height() );
    */
}

unsigned short MoonPhaseCalendar::computeMoonPhase( const KStarsDateTime &date ) {

    KSNumbers num( date.djd() );

    KSPlanet *m_Earth = new KSPlanet( I18N_NOOP( "Earth" ), QString(), QColor( "white" ), 12756.28 /*diameter in km*/ );
    m_Earth->findPosition( &num );

    m_Moon.findGeocentricPosition( &num, m_Earth );
    m_Moon.findPhase();

    return m_Moon.getIPhase();

}
