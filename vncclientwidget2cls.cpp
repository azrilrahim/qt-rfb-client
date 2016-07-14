/*
 * Copyright (C) 2013 Azril Azam
 * <azrilazam@gmail.com>
 *
 * Desc
 *
 * This file is a part of the 100% Qt5 VNC RFB-CLIENT implementation without
 * using any 3rd party rfb library
 *
 * rfbclientwidget2cls.cpp
 *
 *
 * Qt5 RFB-CLIENT is free software: you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public License as
 * published by the Free Software Foundation, either version 3 of the License,
 * or (at your option) any later version.
 *
 * Qt5 RFB-CLIENT is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with Qt HTML platform plugin. If not, see
 * <http://www.gnu.org/licenses/>.
 */

#include "vncclientwidget2cls.h"

vncclientwidget2cls::vncclientwidget2cls(QWidget *parent) :
    QWidget(parent)
{
    QPalette pl;
    this->setAutoFillBackground(true);
    pl.setColor(QPalette::Background,Qt::black);
    this->setPalette(pl);

    this->widgetResizing = false;
    this->rfbWidget.setParent(this);
    this->rfbWidget.hide();


    this->setMinimumSize(640,480);

    connect(&this->rfbWidget,SIGNAL(vncFrameResizeSignal()),this,SLOT(vncFrameResizeSlot()));
    connect(&this->rfbWidget,SIGNAL(rfbClientDisconnectedSignal()),this,SLOT(rfbClientDisconnectedSlot()));
    connect(&this->rfbWidget,SIGNAL(rfbClientPauseSignal()),this,SLOT(rfbClientPauseSlot()));
    connect(&this->rfbWidget,SIGNAL(vncGrabIOSIG()),this,SLOT(rfbClientScreenLockSlot()));
    connect(&this->rfbWidget,SIGNAL(vncUngrabIOSIG()),this,SLOT(rfbClientScreenUnlockSlot()));

    this->rfbWidget.move(0,0);
    this->rfbWidget.show();
    this->resize(640,480);
}

vncclientwidget2cls::~vncclientwidget2cls()
{

    disconnect(&this->rfbWidget,SIGNAL(vncFrameResizeSignal()),this,SLOT(vncFrameResizeSlot()));
    //disconnect(this->rfbWidget,SIGNAL(rfbClientConnectedSignal()),this,SLOT(vncClientConnectedSignal()));
    disconnect(&this->rfbWidget,SIGNAL(rfbClientDisconnectedSignal()),this,SLOT(rfbClientDisconnectedSlot()));
    disconnect(&this->rfbWidget,SIGNAL(rfbClientPauseSignal()),this,SLOT(rfbClientPauseSlot()));
    disconnect(&this->rfbWidget,SIGNAL(vncGrabIOSIG()),this,SLOT(rfbClientScreenLockSlot()));
    disconnect(&this->rfbWidget,SIGNAL(vncUngrabIOSIG()),this,SLOT(rfbClientScreenUnlockSlot()));
}

QImage vncclientwidget2cls::getScreenCapture()
{
    return this->rfbWidget.getScreenCapture();
}

void vncclientwidget2cls::changeEvent(QEvent *E)
{
    //handle min max
    if (E->type() == QEvent::WindowStateChange)
    {
       QWindowStateChangeEvent* event = static_cast< QWindowStateChangeEvent* >( E);

       if( event->oldState() & Qt::WindowMinimized )
            this->updateSize();
       else if ( event->oldState() == Qt::WindowNoState && this->windowState() == Qt::WindowMaximized )
            this->updateSize();   
    }
}

bool vncclientwidget2cls::connectVNCIPC(QString server)
{
    qDebug() << "Connecting VNCIPC at" << server;
    return this->rfbWidget.connectVNCIPC(server);
}

bool vncclientwidget2cls::connectVNCTCP(QString server, qint16 port)
{
    return this->rfbWidget.connectVNCTCP(server,port);
}

void vncclientwidget2cls::disconnectVNC()
{
    this->rfbWidget.disconnectVNC();
}

void vncclientwidget2cls::rfbClientDisconnectedSlot()
{
    emit this->vncClientDisconnectedSignal();
}
void vncclientwidget2cls::rfbClientPauseSlot()
{
    emit this->vncClientPauseSignal();
}

void vncclientwidget2cls::rfbClientConnectedSlot()
{
    emit this->vncClientConnectedSignal();
}

void vncclientwidget2cls::rfbClientScreenLockSlot()
{
    emit this->vncClientScreenLockSignal();
}

void vncclientwidget2cls::rfbClientScreenUnlockSlot()
{
    emit this->vncClientScreenUnlockSignal();
}

void vncclientwidget2cls::resizeEvent(QResizeEvent *)
{
    this->updateSize();
}

void vncclientwidget2cls::closeEvent(QCloseEvent *E)
{
    E->ignore();
    this->disconnectVNC();
    E->accept();
}

void vncclientwidget2cls::vncFrameResizeSlot()
{
    qDebug() << "updateSize";
    this->updateSize();
    emit this->vncFrameResizeEvent();
}

void vncclientwidget2cls::updateSize()
{
    this->widgetResizing = true;

    //resize width to rfbwidget
    if (this->geometry().width() < this->rfbWidget.maximumWidth())
        this->rfbWidget.resize(this->geometry().width(),this->rfbWidget.geometry().height());

    //resize width to maximum
    if (this->geometry().width() >= this->rfbWidget.maximumWidth())
        this->rfbWidget.resize(this->rfbWidget.maximumWidth(),this->rfbWidget.geometry().height());

    //resize height to rfbwidget
    if (this->geometry().height() < this->rfbWidget.maximumHeight())
        this->rfbWidget.resize(this->rfbWidget.geometry().width(),this->geometry().height());

    if (this->geometry().height() >= this->rfbWidget.maximumHeight())
        this->rfbWidget.resize(this->rfbWidget.geometry().width(),this->rfbWidget.maximumHeight());

    if (this->geometry().width() < this->rfbWidget.maximumWidth())
        this->rfbWidget.move(0,this->rfbWidget.y());
    else
    {
        //move to center
        qint32 mX = this->rfbWidget.geometry().width() /2;
        qint32 rw = this->geometry().width()/2;
        qint32 tgtX = rw - mX;
        this->rfbWidget.move(tgtX,this->rfbWidget.y());
    }

    if (this->geometry().height() < this->rfbWidget.maximumHeight())
        this->rfbWidget.move(this->rfbWidget.x(),0);
    else
    {
        qint32 mY = this->rfbWidget.geometry().height()/2;
        qint32 rh = this->geometry().height()/2;
        qint32 tgtY = rh - mY;
        this->rfbWidget.move(this->rfbWidget.x(),tgtY);
    }


    this->widgetResizing = false;

}

bool vncclientwidget2cls::setPauseVNC()
{
    return this->rfbWidget.setPauseRFB();
}

bool vncclientwidget2cls::setResumeVNC()
{
    return this->rfbWidget.setResumeRFB();
}
