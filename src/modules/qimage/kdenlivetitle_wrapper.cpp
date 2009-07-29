/*
 * kdenlivetitle_wrapper.cpp -- kdenlivetitle wrapper
 * Copyright (c) 2009 Marco Gittler <g.marco@freenet.de>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 */

#include "kdenlivetitle_wrapper.h"

#include <QtGui/QImage>
#include <QtGui/QPainter>
#include <QtCore/QDebug>
#include <QtGui/QApplication>
#include <QtCore/QMutex>
#include <QtGui/QGraphicsScene>
#include <QtGui/QGraphicsTextItem>
#include <QtGui/QTextCursor>

static QApplication *app = NULL;

extern "C"
{
  
#include <framework/mlt_producer.h>
#include <framework/mlt_cache.h>

	static QMutex g_mutex;
	void refresh_kdenlivetitle( mlt_producer producer, uint8_t* buffer, int width, int height , double position, int force_refresh )
	{
		if (force_refresh) {
			mlt_properties producer_props = MLT_PRODUCER_PROPERTIES( producer );
			loadFromXml( producer, NULL, mlt_properties_get( producer_props, "xmldata" ), mlt_properties_get( producer_props, "templatetext" ) );
		}
		drawKdenliveTitle( producer, buffer, width, height, position );
	}
	
	static void qscene_delete( void *data )
	{
		QGraphicsScene *scene = ( QGraphicsScene * )data;
		delete scene;
		scene = NULL;
	}
}



void drawKdenliveTitle( mlt_producer producer, uint8_t * buffer, int width, int height, double position )
{
  
  	// restore QGraphicsScene
	g_mutex.lock();
	mlt_cache_item qscene_cache = mlt_service_cache_get( MLT_PRODUCER_SERVICE( producer ), "qscene" );
	QGraphicsScene *scene = static_cast<QGraphicsScene *>( mlt_cache_item_data( qscene_cache, NULL ) );
	mlt_properties producer_props = MLT_PRODUCER_PROPERTIES( producer );
 
	if ( scene == NULL )
	{
		int argc = 1;
		char* argv[1];
		argv[0] = "xxx";
		if (qApp) {
		    app = qApp;
		}
		else {
		    app=new QApplication( argc,argv );
		}
		scene = new QGraphicsScene(0, 0, (qreal) width, (qreal) height, app);
		loadFromXml( producer, scene, mlt_properties_get( producer_props, "xmldata" ), mlt_properties_get( producer_props, "templatetext" ) );
		mlt_service_cache_put( MLT_PRODUCER_SERVICE( producer ), "qscene", scene, 0, ( mlt_destructor )qscene_delete );
	}
	
	g_mutex.unlock();
	
	//must be extracted from kdenlive title
	QImage *img = new QImage( width,height,QImage::Format_ARGB32 );
	img->fill( 0 );
	QPainter p1;
	p1.begin( img );
	p1.setRenderHints( QPainter::Antialiasing | QPainter::TextAntialiasing | QPainter::HighQualityAntialiasing );
	//| QPainter::SmoothPixmapTransform );
	
	QRectF m_start = stringToRect( QString( mlt_properties_get( producer_props, "startrect" ) ) );
	QRectF m_end = stringToRect( QString( mlt_properties_get( producer_props, "endrect" ) ) );
	
	if (m_start.isNull() && m_end.isNull()) {
	    scene->render( &p1 );
	}
	else {
	    QPointF topleft=m_start.topLeft()+( m_end.topLeft()-m_start.topLeft() )*position;
	    QPointF bottomRight=m_start.bottomRight()+( m_end.bottomRight()-m_start.bottomRight() )*position;
	    const QRectF r1( 0, 0, width, height );
	    const QRectF r2( topleft, bottomRight );
	    scene->render( &p1, r1, r2 );
	}
	p1.end();
	uint8_t *pointer=img->bits();
	QRgb* src = ( QRgb* ) pointer;
	for ( int i = 0; i < width * height * 4; i += 4 )
	{
		*buffer++=qRed( *src );
		*buffer++=qGreen( *src );
		*buffer++=qBlue( *src );
		*buffer++=qAlpha( *src );
		src++;
	}
	delete img;
}

void loadFromXml( mlt_producer producer, QGraphicsScene *scene, const char *templateXml, const char *templateText )
{
	if (scene == NULL) {
	  	mlt_cache_item qscene_cache = mlt_service_cache_get( MLT_PRODUCER_SERVICE( producer ), "qscene" );
		scene = static_cast<QGraphicsScene *>( mlt_cache_item_data( qscene_cache, NULL ) );
		if (scene == NULL) return;
	}
	scene->clear();
	mlt_properties producer_props = MLT_PRODUCER_PROPERTIES( producer );
	QDomDocument doc;
	QString data(templateXml);
	QString replacementText(templateText);
	doc.setContent(data);
	QDomNodeList titles = doc.elementsByTagName( "kdenlivetitle" );
	int maxZValue = 0;
	if ( titles.size() )
	{

		QDomNodeList items = titles.item( 0 ).childNodes();
		for ( int i = 0; i < items.count(); i++ )
		{
			QGraphicsItem *gitem = NULL;
			int zValue = items.item( i ).attributes().namedItem( "z-index" ).nodeValue().toInt();
			if ( zValue > -1000 )
			{
				if ( items.item( i ).attributes().namedItem( "type" ).nodeValue() == "QGraphicsTextItem" )
				{
					QDomNamedNodeMap txtProperties = items.item( i ).namedItem( "content" ).attributes();
					QFont font( txtProperties.namedItem( "font" ).nodeValue() );

					QDomNode node = txtProperties.namedItem( "font-bold" );
					if ( !node.isNull() )
					{
						// Old: Bold/Not bold.
						font.setBold( node.nodeValue().toInt() );
					}
					else
					{
						// New: Font weight (QFont::)
						font.setWeight( txtProperties.namedItem( "font-weight" ).nodeValue().toInt() );
					}

					//font.setBold(txtProperties.namedItem("font-bold").nodeValue().toInt());
					font.setItalic( txtProperties.namedItem( "font-italic" ).nodeValue().toInt() );
					font.setUnderline( txtProperties.namedItem( "font-underline" ).nodeValue().toInt() );
					// Older Kdenlive version did not store pixel size but point size
					if ( txtProperties.namedItem( "font-pixel-size" ).isNull() )
					{
						QFont f2;
						f2.setPointSize( txtProperties.namedItem( "font-size" ).nodeValue().toInt() );
						font.setPixelSize( QFontInfo( f2 ).pixelSize() );
					}
					else
						font.setPixelSize( txtProperties.namedItem( "font-pixel-size" ).nodeValue().toInt() );
					QColor col( stringToColor( txtProperties.namedItem( "font-color" ).nodeValue() ) );
					QGraphicsTextItem *txt;
					if ( !replacementText.isEmpty() )
					{
						QString text = items.item( i ).namedItem( "content" ).firstChild().nodeValue();
						text = text.replace( "%s", replacementText );
						txt = scene->addText( text, font );
					}
					else txt = scene->addText( items.item( i ).namedItem( "content" ).firstChild().nodeValue(), font );
					txt->setDefaultTextColor( col );
					txt->setTextInteractionFlags( Qt::NoTextInteraction );
					if ( txtProperties.namedItem( "alignment" ).isNull() == false )
					{
						txt->setTextWidth( txt->boundingRect().width() );
						QTextCursor cur = txt->textCursor();
						QTextBlockFormat format = cur.blockFormat();
						format.setAlignment(( Qt::Alignment ) txtProperties.namedItem( "alignment" ).nodeValue().toInt() );
						cur.select( QTextCursor::Document );
						cur.setBlockFormat( format );
						txt->setTextCursor( cur );
						cur.clearSelection();
						txt->setTextCursor( cur );
					}

					if ( !txtProperties.namedItem( "kdenlive-axis-x-inverted" ).isNull() )
					{
						//txt->setData(OriginXLeft, txtProperties.namedItem("kdenlive-axis-x-inverted").nodeValue().toInt());
					}
					if ( !txtProperties.namedItem( "kdenlive-axis-y-inverted" ).isNull() )
					{
						//txt->setData(OriginYTop, txtProperties.namedItem("kdenlive-axis-y-inverted").nodeValue().toInt());
					}

					gitem = txt;
				}
				else if ( items.item( i ).attributes().namedItem( "type" ).nodeValue() == "QGraphicsRectItem" )
				{
					QString rect = items.item( i ).namedItem( "content" ).attributes().namedItem( "rect" ).nodeValue();
					QString br_str = items.item( i ).namedItem( "content" ).attributes().namedItem( "brushcolor" ).nodeValue();
					QString pen_str = items.item( i ).namedItem( "content" ).attributes().namedItem( "pencolor" ).nodeValue();
					double penwidth = items.item( i ).namedItem( "content" ).attributes().namedItem( "penwidth" ).nodeValue().toDouble();
					QGraphicsRectItem *rec = scene->addRect( stringToRect( rect ), QPen( QBrush( stringToColor( pen_str ) ), penwidth ), QBrush( stringToColor( br_str ) ) );
					gitem = rec;
				}
				else if ( items.item( i ).attributes().namedItem( "type" ).nodeValue() == "QGraphicsPixmapItem" )
				{
					QString url = items.item( i ).namedItem( "content" ).attributes().namedItem( "url" ).nodeValue();
					QPixmap pix( url );
					QGraphicsPixmapItem *rec = scene->addPixmap( pix );
					rec->setData( Qt::UserRole, url );
					gitem = rec;
				}
				else if ( items.item( i ).attributes().namedItem( "type" ).nodeValue() == "QGraphicsSvgItem" )
				{
					QString url = items.item( i ).namedItem( "content" ).attributes().namedItem( "url" ).nodeValue();
					//QGraphicsSvgItem *rec = new QGraphicsSvgItem(url);
					//m_scene->addItem(rec);
					//rec->setData(Qt::UserRole, url);
					//gitem = rec;
				}
			}
			//pos and transform
			if ( gitem )
			{
				QPointF p( items.item( i ).namedItem( "position" ).attributes().namedItem( "x" ).nodeValue().toDouble(),
				           items.item( i ).namedItem( "position" ).attributes().namedItem( "y" ).nodeValue().toDouble() );
				gitem->setPos( p );
				gitem->setTransform( stringToTransform( items.item( i ).namedItem( "position" ).firstChild().firstChild().nodeValue() ) );
				int zValue = items.item( i ).attributes().namedItem( "z-index" ).nodeValue().toInt();
				if ( zValue > maxZValue ) maxZValue = zValue;
				gitem->setZValue( zValue );
				//gitem->setFlags(QGraphicsItem::ItemIsMovable | QGraphicsItem::ItemIsSelectable);
			}
			if ( items.item( i ).nodeName() == "background" )
			{
				QColor color = QColor( stringToColor( items.item( i ).attributes().namedItem( "color" ).nodeValue() ) );
				//color.setAlpha(items.item(i).attributes().namedItem("alpha").nodeValue().toInt());
				QList<QGraphicsItem *> items = scene->items();
				for ( int i = 0; i < items.size(); i++ )
				{
					if ( items.at( i )->zValue() == -1100 )
					{
						(( QGraphicsRectItem * )items.at( i ) )->setBrush( QBrush( color ) );
						break;
					}
				}
			}
			else if ( items.item( i ).nodeName() == "startviewport" )
			{
				QString rect = items.item( i ).attributes().namedItem( "rect" ).nodeValue();
				
				mlt_properties_set( producer_props, "startrect", rect.toUtf8().data() );
			}
			else if ( items.item( i ).nodeName() == "endviewport" )
			{
				QString rect = items.item( i ).attributes().namedItem( "rect" ).nodeValue();
				mlt_properties_set( producer_props, "endrect", rect.toUtf8().data() );
			}
		}
	}
	return;
}

QString colorToString( const QColor& c )
{
	QString ret = "%1,%2,%3,%4";
	ret = ret.arg( c.red() ).arg( c.green() ).arg( c.blue() ).arg( c.alpha() );
	return ret;
}

QString rectFToString( const QRectF& c )
{
	QString ret = "%1,%2,%3,%4";
	ret = ret.arg( c.top() ).arg( c.left() ).arg( c.width() ).arg( c.height() );
	return ret;
}

QRectF stringToRect( const QString & s )
{

	QStringList l = s.split( ',' );
	if ( l.size() < 4 )
		return QRectF();
	return QRectF( l.at( 0 ).toDouble(), l.at( 1 ).toDouble(), l.at( 2 ).toDouble(), l.at( 3 ).toDouble() ).normalized();
}

QColor stringToColor( const QString & s )
{
	QStringList l = s.split( ',' );
	if ( l.size() < 4 )
		return QColor();
	return QColor( l.at( 0 ).toInt(), l.at( 1 ).toInt(), l.at( 2 ).toInt(), l.at( 3 ).toInt() );
	;
}
QTransform stringToTransform( const QString& s )
{
	QStringList l = s.split( ',' );
	if ( l.size() < 9 )
		return QTransform();
	return QTransform(
	           l.at( 0 ).toDouble(), l.at( 1 ).toDouble(), l.at( 2 ).toDouble(),
	           l.at( 3 ).toDouble(), l.at( 4 ).toDouble(), l.at( 5 ).toDouble(),
	           l.at( 6 ).toDouble(), l.at( 7 ).toDouble(), l.at( 8 ).toDouble()
	       );
}
