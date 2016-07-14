/*
 * Copyright (C) 2013 Azril Azam
 * <azrilazam@gmail.com>
 *
 * Desc
 *
 * This file is a part of the 100% Qt5 VNC RFB-CLIENT implementation without
 * using any 3rd party rfb library
 *
 * rfbclientwidgetcls.cpp
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

#include "rfbclientwidgetcls.h"

rfbclientwidgetcls::rfbclientwidgetcls(QWidget *parent) :
    QWidget(parent)
{
    QPalette pl;

    this->vncClient.setParent(this);

    qDebug() << "GLClient init";
    this->setAutoFillBackground(true);
    this->setWindowTitle("Testing");
    this->setMouseTracking(true);
    this->mouseButtonPressed = 0;
    this->mouseX =0;
    this->mouseY = 0;
    pl.setColor(QPalette::Background,Qt::black);
    this->setPalette(pl);


    this->resize(640,480);
    this->setMinimumSize(640,480);
    this->setMaximumSize(640,480);

    this->holdCTRLKey = false;
    this->screenGrab = false;

    connect(&this->vncClient,SIGNAL(rfbResizeSignal(qint16,qint16,QString)),this,SLOT(vncResizeSlot(qint16,qint16,QString)),Qt::DirectConnection);
    connect(&this->vncClient,SIGNAL(rfbFrameBufferUpdateSignal()),this,SLOT(vncFrameBufferUpdateSlot()));
    connect(&this->vncClient,SIGNAL(rfbHostConnectedSignal()),this,SLOT(vncHostConnectedSlot()));
    connect(&this->vncClient,SIGNAL(rfbHostDisconnectedSignal()),this,SLOT(vncHostDisconnectedSlot()));
    connect(&this->vncClient,SIGNAL(rfbPauseSignal()),this,SLOT(vncHostPauseSlot()));

    this->vncConnected = false;
    this->myCursor = this->cursor();

    this->lockedVNC = false;
    this->alreadyHasLockedImage = false;
}

rfbclientwidgetcls::~rfbclientwidgetcls()
{
    qDebug() << "rfbclientwidgetcls is going to be deleted";

        disconnect(&this->vncClient,SIGNAL(rfbResizeSignal(qint16,qint16,QString)),this,SLOT(vncResizeSlot(qint16,qint16,QString)));
        disconnect(&this->vncClient,SIGNAL(rfbFrameBufferUpdateSignal()),this,SLOT(vncFrameBufferUpdateSlot()));
        disconnect(&this->vncClient,SIGNAL(rfbHostConnectedSignal()),this,SLOT(vncHostConnectedSlot()));
        disconnect(&this->vncClient,SIGNAL(rfbHostDisconnectedSignal()),this,SLOT(vncHostDisconnectedSlot()));
        disconnect(&this->vncClient,SIGNAL(rfbPauseSignal()),this,SLOT(vncHostPauseSlot()));

    qDebug() << "rfbclientwidget deleted";
}

bool rfbclientwidgetcls::connectVNCTCP(QString server, qint16 port)
{
    return this->vncClient.connectToHostTCP(server,port);
}

bool rfbclientwidgetcls::connectVNCIPC(QString server)
{
    return this->vncClient.connectToHostIPC(server);
}

void rfbclientwidgetcls::disconnectVNC()
{
    this->vncClient.disconnectFromHost();
}

void rfbclientwidgetcls::vncHostPauseSlot()
{
    emit this->rfbClientPauseSignal();
    this->repaint();
}

void rfbclientwidgetcls::vncHostDisconnectedSlot()
{
    if (this->screenGrab)
    {
        this->screenGrab = false;
        qApp->setOverrideCursor(this->myCursor);
        this->releaseKeyboard();
        this->releaseMouse();
        if (!this->lockedVNC)
            this->resize(640,480);
    }

    qDebug("done");
    this->vncConnected = false;
    emit this->rfbClientDisconnectedSignal();
    this->repaint();

}

void rfbclientwidgetcls::vncHostConnectedSlot()
{
    this->vncConnected = true;
    this->lockedVNC = false;
    this->alreadyHasLockedImage = false;

    this->resize(240,320);
    emit this->rfbClientConnectedSignal();
    qDebug("Widget:: vnc host connected");
}

void rfbclientwidgetcls::vncResizeSlot(qint16 width, qint16 height, QString serverVNCName)
{
    this->setWindowTitle(serverVNCName);
    this->setWindowTitle("NEMD 3.0: WINXP");
    this->oriRFBHeight = height;
    this->oriRFBWidth = width;

    //this->resize(width,height);
    this->setMinimumHeight(480);
    this->setMaximumHeight(height);
    this->setMinimumWidth(640);
    this->setMaximumWidth(width);
    this->resize(width,height);
    emit this->vncFrameResizeSignal();
}

void rfbclientwidgetcls::vncFrameBufferUpdateSlot()
{
    this->repaint();
}

QImage rfbclientwidgetcls::getScreenCapture()
{
    return this->vncClient.getVNCImage();
}

void rfbclientwidgetcls::paintEvent(QPaintEvent *)
{
    QPainter qp(this);
    //qDebug() << "Scaling Img";
    QImage timg = this->vncClient.getVNCImage().scaled(this->geometry().width(),this->geometry().height(),Qt::IgnoreAspectRatio,Qt::SmoothTransformation);
    //QPixmap tpix = this->vncClient->getVNCPixmap().scaled(this->geometry().width(),this->geometry().height(),Qt::IgnoreAspectRatio,Qt::SmoothTransformation);


    //qp.drawPixmap(0,0,tpix);

    qp.drawImage(0,0,timg);
    qp.end();
    //qDebug() << "update image";
}

void rfbclientwidgetcls::closeEvent(QCloseEvent *e)
{
    e->ignore();
    this->vncClient.disconnectFromHost();
    e->accept();
    qDebug("widget close");
}

void rfbclientwidgetcls::keyPressEvent(QKeyEvent *e)
{
    if (this->lockedVNC)
        return;

    if (!this->vncConnected)
        return;

    if (e->key() == Qt::Key_Control)
        this->holdCTRLKey = true;

    if (!this->screenGrab)
        return;

    /*if ((this->holdCTRLKey) && (e->key() == Qt::Key_Alt))
        {
            this->ungrabIO();
            return;
        }*/

        if (e->modifiers() == Qt::NoModifier)
            this->vncClient.sendServerKeyEvent(e->key(),1,false);
        else
            this->vncClient.sendServerKeyEvent(e->key(),1,true);

}

void rfbclientwidgetcls::keyReleaseEvent(QKeyEvent *e)
{
    if (this->lockedVNC)
        return;

    if (!this->vncConnected)
        return;

    if (e->key() == Qt::Key_Control)
        this->holdCTRLKey = false;

    if (!this->screenGrab)
        return;


    if (e->modifiers() == Qt::NoModifier)
        this->vncClient.sendServerKeyEvent(e->key(),0,false);
    else
        this->vncClient.sendServerKeyEvent(e->key(),0,true);
}

void rfbclientwidgetcls::mouseMoveEvent(QMouseEvent *e)
{
    if (this->lockedVNC)
        return;

    if (!this->vncConnected)
        return;

    if (!this->screenGrab)
        return;

    double a = this->oriRFBWidth;
    double b = this->geometry().width();
    double c = this->oriRFBHeight;
    double d = this->geometry().height();
    double fx = (a / b) * e->x();
    double fy = (c / d) * e->y();

    this->mouseX = fx;
    this->mouseY = fy;

    this->vncClient.sendServerPointerEvent(this->mouseX,this->mouseY,this->mouseButtonPressed);
}

void rfbclientwidgetcls::ungrabIO()
{
    this->releaseKeyboard();
    //this->releaseMouse();
    this->screenGrab = false;
    //qApp->setOverrideCursor(this->myCursor);
    this->setCursor(this->myCursor);
    emit this->vncUngrabIOSIG();
}

void rfbclientwidgetcls::grabIO()
{
    this->grabKeyboard();
    //this->grabMouse();
    this->myCursor = this->cursor();
    this->setCursor(QCursor(Qt::UpArrowCursor));
    //qApp->setOverrideCursor(Qt::BlankCursor);
    //this->setCursor(QCursor(Qt::BlankCursor));
    this->screenGrab = true;

    //qDebug() << this->mapToGlobal(QPoint(0,0));

    //this->cursor().setPos(this->mapToGlobal(QPoint(0,0)));
    emit this->vncGrabIOSIG();

}

void rfbclientwidgetcls::mousePressEvent(QMouseEvent *e)
{
    //if (this->lockedVNC)
      //  return;

    if (!this->vncConnected)
        return;

    /*if (!this->screenGrab)
    {
        this->grabIO();
        return;
    }*/

    double a = this->oriRFBWidth;
    double b = this->geometry().width();
    double c = this->oriRFBHeight;
    double d = this->geometry().height();
    double fx = (a / b) * e->x();
    double fy = (c / d) * e->y();

    this->mouseX = fx;
    this->mouseY = fy;

    switch (e->button())
    {
        case Qt::LeftButton:this->mouseButtonPressed = 1;break;
        case Qt::RightButton:this->mouseButtonPressed = 4;break;
        case Qt::MiddleButton: this->mouseButtonPressed = 2;break;
        default: this->mouseButtonPressed = 0;break;
    }

    this->vncClient.sendServerPointerEvent(this->mouseX,this->mouseY,this->mouseButtonPressed);
}

void rfbclientwidgetcls::mouseReleaseEvent(QMouseEvent *e)
{
    if (this->lockedVNC)
        return;

    if (!this->vncConnected)
        return;

    if (!this->screenGrab)
        return;


    double a = this->oriRFBWidth;
    double b = this->geometry().width();
    double c = this->oriRFBHeight;
    double d = this->geometry().height();
    double fx = (a / b) * e->x();
    double fy = (c / d) * e->y();

    this->mouseX = fx;
    this->mouseY = fy;

    /*if (this->mouseX < 0)
        this->mouseX = 0;

    if (this->mouseX > this->geometry().width())
        this->mouseX = this->geometry().width();

    if (this->mouseY < 0)
        this->mouseY = 0;

    if (this->mouseY > this->geometry().height())
        this->mouseY = this->geometry().height();*/

    this->mouseButtonPressed = 0;
    this->vncClient.sendServerPointerEvent(this->mouseX,this->mouseY,this->mouseButtonPressed);
}

void rfbclientwidgetcls::enterEvent(QEvent *)
{
    if (!this->vncConnected)
        return;

    //dont grab if vnc is pause
    if (this->lockedVNC)
        return;
    this->grabIO();
    /*
    this->screenGrab = true;
    this->myCursor = this->cursor();
    //this->setCursor(QCursor(Qt::BlankCursor));
    this->grabKeyboard();
    emit this->vncFrameGetFocusSignal();*/

}

void rfbclientwidgetcls::leaveEvent(QEvent *)
{
    if (!this->vncConnected)
        return;

    if (this->lockedVNC)
        return;

    this->ungrabIO();

   /* this->screenGrab = false;

    if (!this->vncConnected)
        return;

    //this->setCursor(this->myCursor);
    this->releaseKeyboard();*/

}

void rfbclientwidgetcls::resizeEvent(QResizeEvent *)
{

}

bool rfbclientwidgetcls::setPauseRFB()
{
    if (this->vncConnected)
    {
        this->lockedVNC = true;
        if (this->vncClient.pauseRFB())
        {
            this->ungrabIO();
            this->vncConnected = false;
            qDebug() << "repaint for pause";
            this->repaint();
            return true;
        }
    }
    this->lockedVNC = false;
    return false;
}

bool rfbclientwidgetcls::setResumeRFB()
{
    if (!this->vncConnected)
    {
        if (this->vncClient.resumeRFB())
        {
            this->vncConnected = true;
            this->lockedVNC = false;
            qDebug() << "repaint for resume";
            this->repaint();
            return true;
        }
    }
    return false;
}
