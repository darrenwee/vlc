/*****************************************************************************
 * views.cpp : Views for the Playlist
 ****************************************************************************
 * Copyright © 2010 the VideoLAN team
 * $Id$
 *
 * Authors:         Jean-Baptiste Kempf <jb@videolan.org>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston MA 02110-1301, USA.
 *****************************************************************************/

#include "components/playlist/views.hpp"
#include "components/playlist/playlist_model.hpp" /* PLModel */
#include "components/playlist/sorting.h"          /* Columns List */
#include "input_manager.hpp"                      /* THEMIM */

#include <QPainter>
#include <QRect>
#include <QStyleOptionViewItem>
#include <QFontMetrics>
#include <QDrag>
#include <QDragMoveEvent>

#include "assert.h"

/* ICON_SCALER comes currently from harrison-stetson method, so good value */
#define ICON_SCALER         16
#define ART_RADIUS          5
#define SPACER              5

void AbstractPlViewItemDelegate::paintBackground(
    QPainter *painter, const QStyleOptionViewItem & option, const QModelIndex & index ) const
{
    /* FIXME: This does not indicate item selection in all QStyles, so for the time being we
       have to draw it ourselves, to ensure visible effect of selection on all platforms */
    /* QApplication::style()->drawPrimitive( QStyle::PE_PanelItemViewItem, &option,
                                            painter ); */

    painter->save();
    QRect r = option.rect.adjusted( 0, 0, -1, -1 );
    if( option.state & QStyle::State_Selected )
    {
        painter->setBrush( option.palette.color( QPalette::Highlight ) );
        painter->setPen( option.palette.color( QPalette::Highlight ).darker( 150 ) );
        painter->drawRect( r );
    }
    else if( index.data( PLModel::IsCurrentRole ).toBool() )
    {
        painter->setBrush( QBrush( Qt::lightGray ) );
        painter->setPen( QColor( Qt::darkGray ) );
        painter->drawRect( r );
    }
    if( option.state & QStyle::State_MouseOver )
    {
        painter->setOpacity( 0.5 );
        painter->setPen( Qt::NoPen );
        painter->setBrush( option.palette.color( QPalette::Highlight ).lighter( 150 ) );
        painter->drawRect( option.rect );
    }
    painter->restore();
}

void PlIconViewItemDelegate::paint( QPainter * painter, const QStyleOptionViewItem & option, const QModelIndex & index ) const
{
    QString title = PLModel::getMeta( index, COLUMN_TITLE );
    QString artist = PLModel::getMeta( index, COLUMN_ARTIST );

    QFont font( index.data( Qt::FontRole ).value<QFont>() );
    painter->setFont( font );
    QFontMetrics fm = painter->fontMetrics();

    int averagewidth = fm.averageCharWidth();
    int art_width = averagewidth * ICON_SCALER;
    int art_height = averagewidth * ICON_SCALER;

    QPixmap artPix = PLModel::getArtPixmap( index, QSize( art_width, art_height) );

    paintBackground( painter, option, index );

    painter->save();

    QRect artRect( option.rect.x() + averagewidth*2 + ( art_width - artPix.width() ) / 2,
                   option.rect.y() + averagewidth + ( art_height - artPix.height() ) / 2,
                   artPix.width(), artPix.height() );

    // Draw the drop shadow
    painter->save();
    painter->setOpacity( 0.7 );
    painter->setBrush( QBrush( Qt::darkGray ) );
    painter->setPen( Qt::NoPen );
    painter->drawRoundedRect( artRect.adjusted( 0, 0, 2, 2 ), ART_RADIUS, ART_RADIUS );
    painter->restore();

    // Draw the art pixmap
    QPainterPath artRectPath;
    artRectPath.addRoundedRect( artRect, ART_RADIUS, ART_RADIUS );
    painter->setClipPath( artRectPath );
    painter->drawPixmap( artRect, artPix );
    painter->setClipping( false );

    if( option.state & QStyle::State_Selected )
        painter->setPen( option.palette.color( QPalette::HighlightedText ) );


    //Draw children indicator
    if( !index.data( PLModel::IsLeafNodeRole ).toBool() )
    {
        QRect r( option.rect );
        r.setSize( QSize( 25, 25 ) );
        r.translate( 5, 5 );
        if( index.data( PLModel::IsCurrentsParentNodeRole ).toBool() )
        {
            painter->setOpacity( 0.75 );
            QPainterPath nodeRectPath;
            nodeRectPath.addRoundedRect( r, 4, 4 );
            painter->fillPath( nodeRectPath, option.palette.color( QPalette::Highlight ) );
            painter->setOpacity( 1.0 );
        }
        QPixmap dirPix( ":/type/node" );
        QRect r2( dirPix.rect() );
        r2.moveCenter( r.center() );
        painter->drawPixmap( r2, dirPix );
    }

    // Draw title
    font.setItalic( true );

    fm = painter->fontMetrics();
    QRect textRect = option.rect.adjusted( 1, art_height + 10, 0, -1 );
    textRect.setHeight( fm.height() );

    painter->drawText( textRect,
                      fm.elidedText( title, Qt::ElideRight, textRect.width() ),
                      QTextOption( Qt::AlignCenter ) );

    // Draw artist
    painter->setPen( painter->pen().color().lighter( 150 ) );
    font.setItalic( false );
    painter->setFont( font );
    fm = painter->fontMetrics();

    textRect.moveTop( textRect.bottom() + 1 );

    painter->drawText(  textRect,
                        fm.elidedText( artist, Qt::ElideRight, textRect.width() ),
                        QTextOption( Qt::AlignCenter ) );

    painter->restore();
}

QSize PlIconViewItemDelegate::sizeHint ( const QStyleOptionViewItem & option, const QModelIndex & index ) const
{
    QFont f( index.data( Qt::FontRole ).value<QFont>() );
    f.setBold( true );
    QFontMetrics fm( f );
    int textHeight = fm.height();
    int averagewidth = fm.averageCharWidth();
    QSize sz ( averagewidth * ICON_SCALER + 4 * SPACER,
               averagewidth * ICON_SCALER + 4 * SPACER + 2 * textHeight + 1 );
    return sz;
}


#define LISTVIEW_ART_SIZE 45

void PlListViewItemDelegate::paint( QPainter * painter, const QStyleOptionViewItem & option, const QModelIndex & index ) const
{
    QString title = PLModel::getMeta( index, COLUMN_TITLE );
    QString duration = PLModel::getMeta( index, COLUMN_DURATION );
    if( !duration.isEmpty() ) title += QString(" [%1]").arg( duration );

    QString artist = PLModel::getMeta( index, COLUMN_ARTIST );
    QString album = PLModel::getMeta( index, COLUMN_ALBUM );
    QString trackNum = PLModel::getMeta( index, COLUMN_TRACK_NUMBER );
    QString artistAlbum = artist;
    if( !album.isEmpty() )
    {
        if( !artist.isEmpty() ) artistAlbum += ": ";
        artistAlbum += album;
        if( !trackNum.isEmpty() ) artistAlbum += QString( " [#%1]" ).arg( trackNum );
    }

    QPixmap artPix = PLModel::getArtPixmap( index, QSize( LISTVIEW_ART_SIZE, LISTVIEW_ART_SIZE ) );

    //Draw selection rectangle and current playing item indication
    paintBackground( painter, option, index );

    QRect artRect( artPix.rect() );
    artRect.moveCenter( QPoint( artRect.center().x() + 3,
                                option.rect.center().y() ) );
    //Draw album art
    painter->drawPixmap( artRect, artPix );

    //Start drawing text
    painter->save();

    if( option.state & QStyle::State_Selected )
        painter->setPen( option.palette.color( QPalette::HighlightedText ) );

    QTextOption textOpt( Qt::AlignVCenter | Qt::AlignLeft );
    textOpt.setWrapMode( QTextOption::NoWrap );

    QFont f( index.data( Qt::FontRole ).value<QFont>() );

    //Draw title info
    f.setItalic( true );
    painter->setFont( f );
    QFontMetrics fm( painter->fontMetrics() );

    QRect textRect = option.rect.adjusted( LISTVIEW_ART_SIZE + 10, 0, -10, 0 );
    if( !artistAlbum.isEmpty() )
    {
        textRect.setHeight( fm.height() );
        textRect.moveBottom( option.rect.center().y() - 2 );
    }

    //Draw children indicator
    if( !index.data( PLModel::IsLeafNodeRole ).toBool() )
    {
        QPixmap dirPix = QPixmap( ":/type/node" );
        painter->drawPixmap( QPoint( textRect.x(), textRect.center().y() - dirPix.height() / 2 ),
                             dirPix );
        textRect.setLeft( textRect.x() + dirPix.width() + 5 );
    }

    painter->drawText( textRect,
                       fm.elidedText( title, Qt::ElideRight, textRect.width() ),
                       textOpt );

    // Draw artist and album info
    if( !artistAlbum.isEmpty() )
    {
        f.setItalic( false );
        painter->setFont( f );
        fm = painter->fontMetrics();

        textRect.moveTop( textRect.bottom() + 4 );
        textRect.setLeft( textRect.x() + 20 );

        painter->drawText( textRect,
                           fm.elidedText( artistAlbum, Qt::ElideRight, textRect.width() ),
                           textOpt );
    }

    painter->restore();
}

QSize PlListViewItemDelegate::sizeHint ( const QStyleOptionViewItem & option, const QModelIndex & index ) const
{
    QFont f;
    f.setBold( true );
    QFontMetrics fm( f );
    int height = qMax( LISTVIEW_ART_SIZE, 2 * fm.height() + 4 ) + 6;
    return QSize( 0, height );
}

static inline void plViewStartDrag( QAbstractItemView *view, const Qt::DropActions & supportedActions )
{
    QDrag *drag = new QDrag( view );
    drag->setPixmap( QPixmap( ":/noart64" ) );
    drag->setMimeData( view->model()->mimeData(
        view->selectionModel()->selectedIndexes() ) );
    drag->exec( supportedActions );
}

static void plViewDragMoveEvent( QAbstractItemView *view, QDragMoveEvent * event )
{
    if( event->keyboardModifiers() & Qt::ControlModifier &&
        event->possibleActions() & Qt::CopyAction )
        event->setDropAction( Qt::CopyAction );
    else event->acceptProposedAction();
}

PlIconView::PlIconView( PLModel *model, QWidget *parent ) : QListView( parent )
{
    PlIconViewItemDelegate *delegate = new PlIconViewItemDelegate( this );

    setModel( model );
    setViewMode( QListView::IconMode );
    setMovement( QListView::Static );
    setResizeMode( QListView::Adjust );
    setWrapping( true );
    setUniformItemSizes( true );
    setSelectionMode( QAbstractItemView::ExtendedSelection );
    setDragEnabled(true);
    /* dropping in QListView::IconMode does not seem to work */
    //setAcceptDrops( true );
    //setDropIndicatorShown(true);

    setItemDelegate( delegate );
}

void PlIconView::startDrag ( Qt::DropActions supportedActions )
{
    plViewStartDrag( this, supportedActions );
}

void PlIconView::dragMoveEvent ( QDragMoveEvent * event )
{
    plViewDragMoveEvent( this, event );
    QAbstractItemView::dragMoveEvent( event );
}

PlListView::PlListView( PLModel *model, QWidget *parent ) : QListView( parent )
{
    setModel( model );
    setViewMode( QListView::ListMode );
    setUniformItemSizes( true );
    setSelectionMode( QAbstractItemView::ExtendedSelection );
    setAlternatingRowColors( true );
    setDragEnabled(true);
    setAcceptDrops( true );
    setDropIndicatorShown(true);

    PlListViewItemDelegate *delegate = new PlListViewItemDelegate( this );
    setItemDelegate( delegate );
}

void PlListView::startDrag ( Qt::DropActions supportedActions )
{
    plViewStartDrag( this, supportedActions );
}

void PlListView::dragMoveEvent ( QDragMoveEvent * event )
{
    plViewDragMoveEvent( this, event );
    QAbstractItemView::dragMoveEvent( event );
}

void PlListView::keyPressEvent( QKeyEvent *event )
{
    //If the space key is pressed, override the standard list behaviour to allow pausing
    //to proceed.
    if ( event->modifiers() == Qt::NoModifier && event->key() == Qt::Key_Space )
        QWidget::keyPressEvent( event );
    //Otherwise, just do as usual.
    else
        QListView::keyPressEvent( event );
}

void PlTreeView::startDrag ( Qt::DropActions supportedActions )
{
    plViewStartDrag( this, supportedActions );
}

void PlTreeView::dragMoveEvent ( QDragMoveEvent * event )
{
    plViewDragMoveEvent( this, event );
    QAbstractItemView::dragMoveEvent( event );
}

void PlTreeView::keyPressEvent( QKeyEvent *event )
{
    //If the space key is pressed, override the standard list behaviour to allow pausing
    //to proceed.
    if ( event->modifiers() == Qt::NoModifier && event->key() == Qt::Key_Space )
        QWidget::keyPressEvent( event );
    //Otherwise, just do as usual.
    else
        QTreeView::keyPressEvent( event );
}

#include <QHBoxLayout>
PicFlowView::PicFlowView( PLModel *p_model, QWidget *parent ) : QAbstractItemView( parent )
{
    QHBoxLayout *layout = new QHBoxLayout( this );
    layout->setMargin( 0 );
    picFlow = new PictureFlow( this );
    picFlow->setSlideSize(QSize(128,128));
    layout->addWidget( picFlow );
    setSelectionMode( QAbstractItemView::SingleSelection );
    setModel( p_model );

    //CONNECT( picFlow, centerIndexChanged(int), this, playItem(int) );
}

int PicFlowView::horizontalOffset() const
{
    return 0;
}

int PicFlowView::verticalOffset() const
{
    return 0;
}

QRect PicFlowView::visualRect(const QModelIndex &index ) const
{
    return QRect( QPoint(0,0), picFlow->slideSize() );
}

void PicFlowView::scrollTo(const QModelIndex &index, QAbstractItemView::ScrollHint)
{
     int currentIndex = picFlow->centerIndex();
     if( qAbs( currentIndex - index.row()) > 100 )
        picFlow->setCenterIndex( index.row()-1);
     else
        picFlow->showSlide( index.row()-1 );
}

QModelIndex PicFlowView::indexAt(const QPoint &) const
{
    return QModelIndex();
    // No idea, PictureFlow doesn't provide anything to help this
}

QModelIndex PicFlowView::moveCursor(QAbstractItemView::CursorAction action, Qt::KeyboardModifiers)
{
}

bool PicFlowView::isIndexHidden(const QModelIndex &index) const
{
    int currentIndex = picFlow->centerIndex();
    if( index.row()-5 <= currentIndex &&
        index.row()+5 >= currentIndex )
        return false;
    else
        return true;
}

QRegion PicFlowView::visualRegionForSelection(const QItemSelection &) const
{
    return QRect();
}

void PicFlowView::setSelection(const QRect &, QFlags<QItemSelectionModel::SelectionFlag>)
{
    // No selection possible
}

void PicFlowView::rowsInserted(const QModelIndex &parent, int start, int end)
{
    for( int i = start; i <= end; i++ )
    {
        const QModelIndex index = model()->index( i, 0, parent );
        if( !index.isValid() )
            return;

        /* FIXME, this returns no art, so far */
        QPixmap pix = PLModel::getArtPixmap( index, QSize(128,128) );
        picFlow->addSlide(pix);
    }

    picFlow->render();
}

void PicFlowView::playItem( int i_item )
{
    emit activated( model()->index(i_item, 0) );
}

