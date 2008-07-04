#ifndef SINGLE_SOURCE_COMPILE

/*
 * sample_track.cpp - implementation of class sampleTrack, a track which
 *                    provides arrangement of samples
 *
 * Copyright (c) 2005-2008 Tobias Doerffel <tobydox/at/users.sourceforge.net>
 * 
 * This file is part of Linux MultiMedia Studio - http://lmms.sourceforge.net
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public
 * License along with this program (see COPYING); if not, write to the
 * Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301 USA.
 *
 */


#include <Qt/QtXml>
#include <QtGui/QDropEvent>
#include <QtGui/QPainter>
#include <QtGui/QPushButton>

#include "effect_label.h"
#include "sample_track.h"
#include "song.h"
#include "embed.h"
#include "engine.h"
#include "templates.h"
#include "tooltip.h"
#include "audio_port.h"
#include "automation_pattern.h"
#include "sample_play_handle.h"
#include "string_pair_drag.h"
#include "knob.h"



sampleTCO::sampleTCO( track * _track ) :
	trackContentObject( _track ),
	m_sampleBuffer( new sampleBuffer )
{
	saveJournallingState( FALSE );
	setSampleFile( "" );
	restoreJournallingState();

	// we need to receive bpm-change-events, because then we have to
	// change length of this TCO
	connect( engine::getSong(), SIGNAL( tempoChanged( bpm_t ) ),
					this, SLOT( updateLength( bpm_t ) ) );
}




sampleTCO::~sampleTCO()
{
	sharedObject::unref( m_sampleBuffer );
}




void sampleTCO::changeLength( const midiTime & _length )
{
	trackContentObject::changeLength( tMax( static_cast<Sint32>( _length ),
							DefaultTicksPerTact ) );
}




const QString & sampleTCO::sampleFile( void ) const
{
	return( m_sampleBuffer->audioFile() );
}




void sampleTCO::setSampleFile( const QString & _sf )
{
	m_sampleBuffer->setAudioFile( _sf );
	updateLength();

	emit sampleChanged();
}




void sampleTCO::updateLength( bpm_t )
{
	changeLength( sampleLength() );
}




midiTime sampleTCO::sampleLength( void ) const
{
	return( m_sampleBuffer->frames() / engine::framesPerTick() );
}




void sampleTCO::saveSettings( QDomDocument & _doc, QDomElement & _this )
{
	if( _this.parentNode().nodeName() == "clipboard" )
	{
		_this.setAttribute( "pos", -1 );
	}
	else
	{
		_this.setAttribute( "pos", startPosition() );
	}
	_this.setAttribute( "len", length() );
	_this.setAttribute( "muted", isMuted() );
	_this.setAttribute( "src", sampleFile() );
	if( sampleFile() == "" )
	{
		QString s;
		_this.setAttribute( "data", m_sampleBuffer->toBase64( s ) );
	}
	// TODO: start- and end-frame
}




void sampleTCO::loadSettings( const QDomElement & _this )
{
	if( _this.attribute( "pos" ).toInt() >= 0 )
	{
		movePosition( _this.attribute( "pos" ).toInt() );
	}
	setSampleFile( _this.attribute( "src" ) );
	if( sampleFile().isEmpty() && _this.hasAttribute( "data" ) )
	{
		m_sampleBuffer->loadFromBase64( _this.attribute( "data" ) );
	}
	changeLength( _this.attribute( "len" ).toInt() );
	setMuted( _this.attribute( "muted" ).toInt() );
}




trackContentObjectView * sampleTCO::createView( trackView * _tv )
{
	return( new sampleTCOView( this, _tv ) );
}










sampleTCOView::sampleTCOView( sampleTCO * _tco, trackView * _tv ) :
	trackContentObjectView( _tco, _tv ),
	m_tco( _tco )
{
	connect( m_tco, SIGNAL( sampleChanged() ),
			this, SLOT( updateSample() ) );
}




sampleTCOView::~sampleTCOView()
{
}




void sampleTCOView::updateSample( void )
{
	update();
	// set tooltip to filename so that user can see what sample this
	// sample-tco contains
	toolTip::add( this, ( m_tco->m_sampleBuffer->audioFile() != "" ) ?
					m_tco->m_sampleBuffer->audioFile() :
					tr( "double-click to select sample" ) );
}




void sampleTCOView::dragEnterEvent( QDragEnterEvent * _dee )
{
	if( stringPairDrag::processDragEnterEvent( _dee,
					"samplefile,sampledata" ) == FALSE )
	{
		trackContentObjectView::dragEnterEvent( _dee );
	}
}




void sampleTCOView::dropEvent( QDropEvent * _de )
{
	if( stringPairDrag::decodeKey( _de ) == "samplefile" )
	{
		m_tco->setSampleFile( stringPairDrag::decodeValue( _de ) );
		_de->accept();
	}
	else if( stringPairDrag::decodeKey( _de ) == "sampledata" )
	{
		m_tco->m_sampleBuffer->loadFromBase64(
					stringPairDrag::decodeValue( _de ) );
		m_tco->updateLength();
		update();
		_de->accept();
		engine::getSong()->setModified();
	}
	else
	{
		trackContentObjectView::dropEvent( _de );
	}
}




void sampleTCOView::mouseDoubleClickEvent( QMouseEvent * )
{
	QString af = m_tco->m_sampleBuffer->openAudioFile();
	if( af != "" && af != m_tco->m_sampleBuffer->audioFile() )
	{
		m_tco->setSampleFile( af );
		engine::getSong()->setModified();
	}
}




void sampleTCOView::paintEvent( QPaintEvent * _pe )
{
	QPainter p( this );

	QLinearGradient grad( 0, 0, 0, height() );
	if( isSelected() )
	{
		grad.setColorAt( 1, QColor( 0, 0, 224 ) );
		grad.setColorAt( 0, QColor( 0, 0, 128 ) );
	}
	else
	{
		grad.setColorAt( 0, QColor( 96, 96, 96 ) );
		grad.setColorAt( 1, QColor( 16, 16, 16 ) );
	}
	p.fillRect( _pe->rect(), grad );

	p.setPen( QColor( 0, 0, 0 ) );
	p.drawRect( 0, 0, width()-1, height()-1 );
	if( m_tco->getTrack()->isMuted() || m_tco->isMuted() )
	{
		p.setPen( QColor( 128, 128, 128 ) );
	}
	else
	{
		p.setPen( QColor( 64, 224, 160 ) );
	}
	QRect r = QRect( 1, 1,
			tMax( static_cast<int>( m_tco->sampleLength() *
				pixelsPerTact() / DefaultTicksPerTact ), 1 ),
								height() - 4 );
	p.setClipRect( QRect( 1, 1, width() - 2, height() - 2 ) );
	m_tco->m_sampleBuffer->visualize( p, r, _pe->rect() );
	if( r.width() < width() - 1 )
	{
		p.drawLine( r.x() + r.width(), r.y() + r.height() / 2,
				width() - 2, r.y() + r.height() / 2 );
	}

	p.translate( 0, 0 );
	if( m_tco->isMuted() )
	{
		p.drawPixmap( 3, 8, embed::getIconPixmap( "muted", 16, 16 ) );
	}
}






sampleTrack::sampleTrack( trackContainer * _tc ) :
	track( SampleTrack, _tc ),
	m_audioPort( tr( "Sample track" ) ),
	m_volumeModel( DefaultVolume, MinVolume, MaxVolume, 1.0, this,
							tr( "Volume" ) )
{
	setName( tr( "Sample track" ) );
}




sampleTrack::~sampleTrack()
{
	engine::getMixer()->removePlayHandles( this );
}




bool sampleTrack::play( const midiTime & _start, const fpp_t _frames,
						const f_cnt_t _offset,
							Sint16 /*_tco_num*/ )
{
	m_audioPort.getEffects()->startRunning();
	bool played_a_note = FALSE;	// will be return variable

	for( int i = 0; i < numOfTCOs(); ++i )
	{
		trackContentObject * tco = getTCO( i );
		if( tco->startPosition() != _start )
		{
			continue;
		}
		sampleTCO * st = dynamic_cast<sampleTCO *>( tco );
		if( !st->isMuted() )
		{
			samplePlayHandle * handle = new samplePlayHandle( st );
			handle->setVolumeModel( &m_volumeModel );
//TODO: check whether this works
//			handle->setBBTrack( _tco_num );
			handle->setOffset( _offset );
			// send it to the mixer
			engine::getMixer()->addPlayHandle( handle );
			played_a_note = TRUE;
		}
	}

	return( played_a_note );
}




trackView * sampleTrack::createView( trackContainerView * _tcv )
{
	return( new sampleTrackView( this, _tcv ) );
}




trackContentObject * sampleTrack::createTCO( const midiTime & )
{
	return( new sampleTCO( this ) );
}




void sampleTrack::saveTrackSpecificSettings( QDomDocument & _doc,
							QDomElement & _this )
{
	m_audioPort.getEffects()->saveState( _doc, _this );
#if 0
	_this.setAttribute( "icon", m_trackLabel->pixmapFile() );
#endif
	m_volumeModel.saveSettings( _doc, _this, "vol" );
}




void sampleTrack::loadTrackSpecificSettings( const QDomElement & _this )
{
	QDomNode node = _this.firstChild();
	m_audioPort.getEffects()->clear();
	while( !node.isNull() )
	{
		if( node.isElement() )
		{
			if( m_audioPort.getEffects()->nodeName() ==
							node.nodeName() )
			{
				m_audioPort.getEffects()->restoreState(
							node.toElement() );
			}
		}
		node = node.nextSibling();
	}
#if 0
	if( _this.attribute( "icon" ) != "" )
	{
		m_trackLabel->setPixmapFile( _this.attribute( "icon" ) );
	}
#endif
	m_volumeModel.loadSettings( _this, "vol" );
}






sampleTrackView::sampleTrackView( sampleTrack * _t, trackContainerView * _tcv ) :
	trackView( _t, _tcv )
{
	setFixedHeight( 32 );

	m_trackLabel = new effectLabel( getTrackSettingsWidget(), _t );
#if 0
	m_trackLabel = new nameLabel( tr( "Sample track" ),
						getTrackSettingsWidget() );
	m_trackLabel->setPixmap( embed::getIconPixmap( "sample_track" ) );
#endif
	m_trackLabel->setGeometry( 26, 1, DEFAULT_SETTINGS_WIDGET_WIDTH-2, 29 );
	m_trackLabel->show();

	m_volumeKnob = new knob( knobSmall_17, getTrackSettingsWidget(),
						    tr( "Track volume" ) );
	m_volumeKnob->setVolumeKnob( TRUE );
	m_volumeKnob->setModel( &_t->m_volumeModel );
	m_volumeKnob->setHintText( tr( "Channel volume:" ) + " ", "%" );
	m_volumeKnob->move( 4, 4 );
	m_volumeKnob->setLabel( tr( "VOL" ) );
	m_volumeKnob->show();
}




sampleTrackView::~sampleTrackView()
{
}




#include "sample_track.moc"


#endif
