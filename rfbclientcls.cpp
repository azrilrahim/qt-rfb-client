/*
 * Copyright (C) 2013 Azril Azam
 * <azrilazam@gmail.com>
 *
 * Desc
 *
 * This file is a part of the 100% Qt5 VNC RFB-CLIENT implementation without
 * using any 3rd party rfb library
 *
 * rfbclientcls.cpp
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

#include "rfbclientcls.h"

rfbclientcls::rfbclientcls(QObject *parent) :
    QObject(parent)
{
    pixelFormatStruct pixel_format;


    this->vncClientTCP.setParent(this);
    this->vncClientIPC.setParent(this);
    this->opsThreadTimer.setParent(this);

    this->serverConnected = false;
    this->socketMode = 10; //default TCP
    this->pauseMode = false;
    xEmitDisconnecSignal = false;

    //set default client pixel information
    pixel_format.bpp         = 32;
    pixel_format.depth       = 24;
    pixel_format.true_color_flag  = 1;
    pixel_format.big_endian_flag  = 0;
    pixel_format.red_max     = 0xFF;
    pixel_format.green_max   = 0xFF;
    pixel_format.blue_max    = 0xFF;
    pixel_format.red_shift   = 0x10;
    pixel_format.green_shift = 0x08;
    pixel_format.blue_shift  = 0x00;

    qDebug("Setting client pixel");
    this->clientPixelF = pixel_format;

    qDebug("Setting client color format");
    this->clientColorFormat = this->getColorFormat(&this->clientPixelF);

    qDebug("load default image");
    this->VNCIMAGE = QImage(":/new/prefix1/creditimg.png");
}


rfbclientcls::~rfbclientcls()
{
    qDebug() << "rfbclientcls is going to be deleted";

    this->disconnectFromHost();
    /*this->opsThreadTimer.stop();
    disconnect(&this->opsThreadTimer,SIGNAL(timeout()),this,SLOT(opsThreadTimerTimeOutSlot()));

    this->vncClientTCP.abort();
    this->vncClientIPC.abort();
    this->opsThreadTimer.stop();
    disconnect(&this->opsThreadTimer,SIGNAL(timeout()),this,SLOT(opsThreadTimerTimeOutSlot()));*/
    /*if (this->opsThreadTimer != NULL)
    {

        disconnect(this->opsThreadTimer,SIGNAL(timeout()),this,SLOT(opsThreadTimerTimeOutSlot()));
        delete this->opsThreadTimer;
        this->opsThreadTimer = NULL;
    }

    if (this->vncClientTCP != NULL)
    {
        //disconnect(this->vncClientTCP,SIGNAL(disconnected()),this,SLOT(vncSockDisconnectedSlot()));
        this->vncClientTCP->abort();
        delete this->vncClientTCP;
        this->vncClientTCP = NULL;
    }

    if (this->vncClientIPC != NULL)
    {
        //disconnect(this->vncClientIPC,SIGNAL(disconnected()),this,SLOT(vncSockDisconnectedSlot()));
        this->vncClientIPC->abort();
        delete this->vncClientIPC;
        this->vncClientIPC = NULL;
    }*/

    qDebug("rfbclientcls deleted");
}

void rfbclientcls::opsThreadTimerTimeOutSlot()
{
        this->processRFBOperationProtocol();
}

void rfbclientcls::processRFBOperationProtocol()
{

    switch (this->socketMode){
    case 0:
        if (this->vncClientTCP.state() != QTcpSocket::ConnectedState)
        {
            this->disconnectFromHost();
            return;
        }
        break;

    case 1:
        if (this->vncClientIPC.state() != QLocalSocket::ConnectedState)
        {
            this->disconnectFromHost();
            return;
        }
        break;
    }

    this->getRFBOpsType();
    this->opsThreadTimer.start(1);

}

void rfbclientcls::newUpdateRFBFB()
{
    unsigned char padding;
    quint16 totalRects;
    quint16 currentRect;
    rfbRectHeader rfbRH;
    qint64 bppSize;
    quint64 totalPixelSize;
    quint32 cursorSize;

    unsigned char *pixelData;
    unsigned char *cursorData;

    unsigned char *px;
    unsigned char *p;
    QImage srcImg;


    //get the padding
    if (this->readFromRFBServer(&padding,1) != 1)
        return;

    //get number of rectangle
    if (this->readFromRFBServer((unsigned char*)&totalRects,2) != 2)
        return;
    totalRects = this->swap16(totalRects);

    //process each rect
    for (currentRect = 0; currentRect < totalRects; currentRect++)
    {
        //process all events;
        qApp->processEvents();

        switch (this->socketMode)
        {
        case 0:
            if (this->vncClientTCP.state() != QTcpSocket::ConnectedState)
                return;
            break;

        case 1:
            if (this->vncClientIPC.state() != QLocalSocket::ConnectedState)
                return;
        }

        //get rect information
        if (this->readFromRFBServer((unsigned char*)&rfbRH,sizeof(rfbRH)) == sizeof(rfbRH))
        {

            //swap the value
            rfbRH.x = swap16(rfbRH.x);
            rfbRH.y = swap16(rfbRH.y);
            rfbRH.width = swap16(rfbRH.width);
            rfbRH.height = swap16(rfbRH.height);
            rfbRH.encoding = swap32(rfbRH.encoding);

            if (rfbRH.width * rfbRH.height > 0)
            {

                switch (rfbRH.encoding)
                {
                    case 0: //Get Raw Data
                        //Get Pixel Size
                        bppSize = (this->serverPixelF.bpp >> 3);
                        totalPixelSize = rfbRH.width * rfbRH.height * bppSize;

                        //create pixel buffer
                        pixelData = (unsigned char*)malloc(totalPixelSize);
                        if (this->readFromRFBServer(pixelData,totalPixelSize) == totalPixelSize)
                        {
                            //create new image
                            px = 0;
                            p = 0;
                            srcImg = QImage(rfbRH.width,rfbRH.height,QImage::Format_RGB888);

                            px = pixelData;
                            p = (unsigned char*)srcImg.bits();
                            for (int cy = 0; cy < rfbRH.height; cy++)
                            {
                                qApp->processEvents();
                                for (int cx = 0; cx < rfbRH.width; cx++)
                                {
                                    p[0]    = px[2];
                                    p[1]    = px[1];
                                    p[2]    = px[0];
                                    px+=4;
                                    p+=3;
                                }
                            }

                            //update image raster
                            QPainter painter(&this->VNCIMAGE);
                            painter.drawImage(rfbRH.x,rfbRH.y,srcImg);
                            painter.end();
                        }
                        //clear pixel data
                        free (pixelData);
                        break;


                    case -223: //this->updateFBGetDesktopSizePseudo();
                        this->VNCIMAGE = QImage(rfbRH.width,rfbRH.height,QImage::Format_RGB888);
                        this->serverFBHeight = rfbRH.height;
                        this->serverFBWidth = rfbRH.width;
                        emit this->rfbResizeSignal(rfbRH.width,rfbRH.height,this->serverVNCName);
                        break;


                    case -239://this->updateFBGetCursorPseudo();

                        cursorSize = rfbRH.width * rfbRH.height;
                        if (cursorSize > 0)
                        {
                            cursorData = (unsigned char *)malloc(cursorSize);
                            if (this->readFromRFBServer(cursorData,cursorSize) == cursorSize)
                                //emit this->rfbCursorPositionSignal(rfbRH.x,rfbRH.y);
                            free(cursorData);
                        }
                        break;
                }
            }
        }
        //process next rectangle
    }

    //if all good then display the image
    if ((currentRect >= totalRects) && (!this->pauseMode))
        emit this->rfbFrameBufferUpdateSignal();

}

void rfbclientcls::getRFBOpsType()
{
    unsigned char msgType;


    if (this->readFromRFBServer((unsigned char*)&msgType,1) != 1)
        return;

    switch (msgType)
    {
        case 0: this->newUpdateRFBFB();break;
        //case 0: this->RFBOpsStage = 1;break; //framebufferupdate
       // case 1: this->RFBOpsStage = 2;break; //setcolormapentries
       // case 2: this->RFBOpsStage = 3; break; //ServerBell
      //  case 3: this->RFBOpsStage = 4; break; //server cut text
    }
    return;
}


/*void rfbclientcls::updateFBGetDesktopSizePseudo(quint16 w, quint16 h)
{


    this->VNCIMAGE = QImage(w,h,QImage::Format_RGB888);
    this->serverFBHeight = w;
    this->serverFBWidth = h;
    emit this->rfbResizeSignal(w,h,this->serverVNCName);

}

void rfbclientcls::updateFBGetCursorPseudo()
{

    unsigned char *cursorData;
    quint32 size = this->gRFBRH.width * this->gRFBRH.height;

    if ((size) <=0)
    {
        this->RFBOpsStage = 13;
        return;
    }

    cursorData = (unsigned char*)malloc(size);
    if (this->readFromRFBServer(cursorData,size) != size)
    {
        free (cursorData);
        return; //read until equal to size;
    }

    //emit this->rfbCursorPositionSignal(this->gRFBRH.x,this->gRFBRH.y);

    free (cursorData);

}*/

quint64 rfbclientcls::readFromRFBServer(unsigned char *data, quint64 size)
{
    //qint64 bSize;
    while(1)
    {
        qApp->processEvents();
        switch (this->socketMode)
        {
            case 0:
            if (this->vncClientTCP.state() != QTcpSocket::ConnectedState)
                return 0;

            if ((quint64)this->vncClientTCP.bytesAvailable() >= size)
            {
                this->vncClientTCP.read((char*)data,(qint64)size);
                return size;
            }
            break;


            case 1:
            if (this->vncClientIPC.state() != QLocalSocket::ConnectedState)
                return 0;

            if ((quint64)this->vncClientIPC.bytesAvailable() >= size)
            {
                this->vncClientIPC.read((char*)data,(qint64)size);
                return size;
            }
            break;
        }
    }

    return 0;
}


QImage rfbclientcls::getVNCImage()
{
    return this->VNCIMAGE;
}

bool rfbclientcls::pauseRFB()
{
    this->pauseMode = true;
    //make image black in white

    this->pauseResumeImg = this->VNCIMAGE;
    this->VNCIMAGE = this->VNCIMAGE.convertToFormat(QImage::Format_Mono);
    this->disconnectFromHost();

    emit this->rfbPauseSignal();
    return true;
}

bool rfbclientcls::resumeRFB()
{
    this->VNCIMAGE = this->pauseResumeImg;//this->VNCIMAGE.convertToFormat(QImage::Format_RGB888);
    this->pauseMode = false;
    return true;
    switch(socketMode)
    {
    case 0:
         if (this->connectToHostTCP(this->serverName,this->serverPort))
         {
            this->pauseMode = false;
            return true;
         }
         break;

    case 1:
        if (this->connectToHostIPC(this->serverName))
        {
            this->pauseMode = false;
            return true;
        }
        break;

    }
    return false;

    /*if (this->serverConnected)
    {
        this->sendClientFrameBufferRequestUpdate(0,0,this->VNCIMAGE.width(),this->VNCIMAGE.height(),0);
        this->pauseMode = false;
        return true;
    }
    return false;*/

}

qint32 rfbclientcls::getColorFormat(pixelFormatStruct *pixel)
{
   // qDebug() << "   check color format for following parameter:";
   // qDebug() << "   ... - BPP = " << pixel->bpp << ", Depth = " << pixel->depth << ", True color = " << pixel->true_color_flag;
   // qDebug() << "   ... - Red max = " << pixel->red_max << ", Green max = " << pixel->green_max << ", Blue max = " << pixel->blue_max;
   // qDebug() << "   ... - Red shift = " << pixel->red_shift << ", Green shift = " << pixel->green_shift << ", Blue shift = " << pixel->blue_shift;

    if (pixel->bpp       == 32   && pixel->depth       == 24   && pixel->true_color_flag == 1    &&
        pixel->red_max   == 0xFF && pixel->green_max   == 0xFF && pixel->blue_max   == 0xFF &&
        pixel->red_shift == 0x10 && pixel->green_shift == 0x08 && pixel->blue_shift == 0x00) {
       // qDebug() << "   color format identified as RGB888";
        return PIXEL_FORMAT_RGB_888;
    }

    if (pixel->bpp       == 16   && pixel->depth       == 16   && pixel->true_color_flag == 1    &&
        pixel->red_max   == 0x1F && pixel->green_max   == 0x3F && pixel->blue_max   == 0x1F &&
        pixel->red_shift == 0x0B && pixel->green_shift == 0x05 && pixel->blue_shift == 0x00) {
       // qDebug() << "   color format identified as RGB565";
        return PIXEL_FORMAT_RGB_565;
    }

    if (pixel->bpp       == 16   && pixel->depth       == 15   && pixel->true_color_flag == 1    &&
        pixel->red_max   == 0x1F && pixel->green_max   == 0x1F && pixel->blue_max   == 0x1F &&
        pixel->red_shift == 0x0A && pixel->green_shift == 0x05 && pixel->blue_shift == 0x00) {
      //  qDebug()<< "   color format identified as RGB555";
        return PIXEL_FORMAT_RGB_555;
    }

    qDebug() << "   ERROR - unknown Color format";

    return PIXEL_FORMAT_NONE;
}

void rfbclientcls::vncSockDisconnectedSlot()
{
    this->disconnectFromHost();
    return;

}

bool rfbclientcls::connectToHostTCP(QString server, qint16 port)
{
    this->socketMode = 0;
    return this->connectToHost(server,port);

}

bool rfbclientcls::connectToHostIPC(QString server)
{
    this->socketMode = 1;
    return this->connectToHost(server,0);
}

bool rfbclientcls::connectToHost(QString server, qint16 port)
{
    this->serverName = server;
    this->serverPort = port;
    QFile sf;

    this->opsThreadTimer.stop();
    disconnect(&this->opsThreadTimer,SIGNAL(timeout()),this,SLOT(opsThreadTimerTimeOutSlot()));

    disconnect(&this->vncClientTCP,SIGNAL(disconnected()),this,SLOT(vncSockDisconnectedSlot()));
    this->vncClientTCP.abort();

    disconnect(&this->vncClientIPC,SIGNAL(disconnected()),this,SLOT(vncSockDisconnectedSlot()));
    this->vncClientIPC.abort();

    //restart fresh
    switch (this->socketMode){

    case 0:
        connect(&this->vncClientTCP,SIGNAL(disconnected()),this,SLOT(vncSockDisconnectedSlot()));
        this->vncClientTCP.connectToHost(server,port);
        if (!this->vncClientTCP.waitForConnected(3000))
        {
            qDebug() << "rfbclientcls: fail to connect to host." << this->vncClientTCP.errorString();
            this->disconnectFromHost();
            return false;
        }
        break;

    case 1:

        //check if local server IPC file exists
        sf.setFileName(server);
        if (!sf.exists())
        {
            qDebug() << "VNC Connect: IPC Server not exists! Exiting";
            return false;
        }
        connect(&this->vncClientIPC,SIGNAL(disconnected()),this,SLOT(vncSockDisconnectedSlot()));
        this->vncClientIPC.connectToServer(server);
        if (!this->vncClientIPC.waitForConnected(3000))
        {
            qDebug() << "rfbclientcls: fail to connect to host." << this->vncClientTCP.errorString();
            this->disconnectFromHost();
            return false;
        }
    }

    qDebug("vnc sock connected");
    this->serverConnected = true;
    this->pauseMode = false;

    emit this->rfbHostConnectedSignal();
    if (!this->performHandshakingProtocol())
    {
        this->disconnectFromHost();
        return false;
    }

    qDebug() << "Timer start";
    this->opsThreadTimer.setSingleShot(true);
    connect(&this->opsThreadTimer,SIGNAL(timeout()),this,SLOT(opsThreadTimerTimeOutSlot()),Qt::DirectConnection);
    this->opsThreadTimer.start(1);
    return true;

}

void rfbclientcls::disconnectFromHost()
{
    qint16 w,h;

    this->opsThreadTimer.stop();
    disconnect(&this->opsThreadTimer,SIGNAL(timeout()),this,SLOT(opsThreadTimerTimeOutSlot()));


    disconnect(&this->vncClientTCP,SIGNAL(disconnected()),this,SLOT(vncSockDisconnectedSlot()));
    this->vncClientTCP.abort();

    disconnect(&this->vncClientIPC,SIGNAL(disconnected()),this,SLOT(vncSockDisconnectedSlot()));
    this->vncClientIPC.abort();

     qDebug("sock disconnected");
     this->serverConnected = false;

     if (!this->pauseMode)
     {
        //if disconnection request is not from pause
        this->VNCIMAGE = QImage(":/new/prefix1/creditimg.png");
        w = this->VNCIMAGE.width();
        h = this->VNCIMAGE.height();
        emit this->rfbResizeSignal(w,h,"");
     }

     //if disconnect request from pause.
     //then reset the pause
     this->pauseMode = false;

     emit this->rfbHostDisconnectedSignal();
}


bool rfbclientcls::performHandshakingProtocol()
{
    if (!this->handleVersionProtocol())
        return false;
    if (!this->handleSecurityProtocol())
        return false;
    if (!this->handleClientInitProtocol())
        return false;
    if (!this->handleServerInitProtocol())
        return false;

    return true;
}

bool rfbclientcls::handleVersionProtocol()
{
    unsigned char vncServerData[12];
    QString enc;



    qDebug("Handling version protocol");
    //step 1. Get the version protocol from server

    if (this->readFromHost((unsigned char*)vncServerData,12) != 12)
        return false;

    enc.clear();
    this->serverMajorVersion = enc.append((QChar)vncServerData[6]).toInt();

    enc.clear();
    this->serverMinorVersion = enc.append((QChar)vncServerData[10]).toInt();

    qDebug("   server version %d.%d",serverMajorVersion,serverMinorVersion);

    //client to server
    enc.clear();
    this->clientMinorVersion = 0;
    if (this->serverMajorVersion >= 3)
    {
        this->clientMajorVersion = 3;
        switch (this->serverMinorVersion)
        {
            case 3: this->clientMinorVersion = 3; enc.append("RFB 003.003\n");break;
            case 7: this->clientMinorVersion = 7; enc.append("RFB 003.007\n");break;
            case 8: this->clientMinorVersion = 8; enc.append("RFB 003.008\n");break;
        }
    }

    if (this->clientMinorVersion == 0)
        return false;

    //test
    //enc.clear();
    //enc.append("RFB 004.000\n");
    //send data to server
    qint32 writers = this->writeToHost((unsigned char*)enc.toStdString().c_str(),12);

    if (writers == 12)
        return true;

    return false;
}

bool rfbclientcls::handleSecurityProtocol()
{
    unsigned char vncServerData[20];
    unsigned char vncClientData[10];
    QString enc;

    qDebug("Handling security protocol");

    //step 1. Get the version protocol from server

    if (this->readFromHost(vncServerData,2) != 2)
        return false;


    qint8 numberOfSecurity = 0;
    qint8 securityID = vncServerData[1];

    numberOfSecurity = vncServerData[0];
    numberOfSecurity = numberOfSecurity;
    //get security info
    //qDebug() << "   number of security " << numberOfSecurity;
    //qDebug() << "   security id" << securityID;

    switch (securityID)
    {
        //no security
        case 0x01:vncClientData[0] = 0x01;qDebug("   Server requires no security");break;
        case 0x02:qDebug("   Requires VNC authentication");break;
        default:qDebug("   unknown security id (%d)",vncServerData[1]);
    }

    //send to server for security cleareance
    qDebug("    requesting no security option");
    if (this->writeToHost((unsigned char*)vncClientData,1) != 1)
        return false;

    qDebug("   waiting for security clearence");
    //wait for server acknowledgement

    if (this->readFromHost((unsigned char*)vncServerData,4) != 4)
        return false;

    if (enc.fromLocal8Bit((char *)vncServerData,4).toInt() == 0)
    {
        qDebug("    security pass");
        return true;
    }

    qDebug("   Security fail");
    return false;
}

bool rfbclientcls::handleClientInitProtocol()
{
    QString vncClientData;


    qDebug("Handling Client Init Protocol");


    vncClientData.clear();
    vncClientData.append("\x01");
    qDebug("   writing server a share flag");
    if (this->writeToHost((unsigned char*)vncClientData.toStdString().c_str(),1) != 1)
        return false;

    return true;
}

bool rfbclientcls::handleServerInitProtocol()
{
    rfbServerInitStruct rfbSID;
    unsigned char *serverName = 0;
    //unsigned char vncServerData[26];


    qDebug("Handling Server Init Protocol");


    if (this->readFromHost((unsigned char*)&rfbSID,sizeof(rfbSID)) != sizeof(rfbSID))
        return false;

    rfbSID.fbWidth = swap16(rfbSID.fbWidth);
    rfbSID.fbHeight = swap16(rfbSID.fbHeight);
    rfbSID.fbPixel.red_max = swap16(rfbSID.fbPixel.red_max);
    rfbSID.fbPixel.green_max = swap16(rfbSID.fbPixel.green_max);
    rfbSID.fbPixel.blue_max = swap16(rfbSID.fbPixel.blue_max);
    rfbSID.fbNameLength = swap32(rfbSID.fbNameLength);


    //set server frame buffer information
    qDebug("   setting server frame buffer information");
    qDebug() << "Height" << rfbSID.fbHeight;
    qDebug() << "Width" << rfbSID.fbWidth;
    //qDebug() << "pixel" << rfbSID.fbPixel.;

    this->serverFBHeight = rfbSID.fbHeight;
    this->serverFBWidth = rfbSID.fbWidth;
    this->serverPixelF = rfbSID.fbPixel;
    this->serverColorFormat = this->getColorFormat(&this->serverPixelF);


    //read server name
    serverName = (unsigned char*)malloc(rfbSID.fbNameLength+1);
    qDebug() << "   reading server name for" << rfbSID.fbNameLength;
    if (this->readFromHost(serverName,rfbSID.fbNameLength) != rfbSID.fbNameLength)
        return false;
    serverName[rfbSID.fbNameLength] = 0x00;
    this->serverVNCName = QString((char*)serverName).trimmed();

    //create a new VNC Image;
    this->VNCIMAGE = QImage(rfbSID.fbWidth,rfbSID.fbHeight,QImage::Format_RGB888);
    this->serverFBHeight = rfbSID.fbHeight;
    this->serverFBWidth = rfbSID.fbWidth;
    emit this->rfbResizeSignal(rfbSID.fbWidth,rfbSID.fbHeight,this->serverVNCName);

    //this->updateFBGetDesktopSizePseudo(rfbSID.fbWidth,rfbSID.fbHeight);

    qDebug("SERVER NAME IS %s", this->serverVNCName.toStdString().c_str());

    free (serverName);

    //create vncImage
    //qDebug("   Creating new RFB Image QImage RGB888");
    //this->VNCIMAGE = QImage(this->serverFBWidth,this->serverFBHeight,QImage::Format_RGB888);

    //check if serverColorFormat == clientColorFormat
    if (this->serverColorFormat != this->clientColorFormat)
    {
        //set server color format
        qDebug("   Resetting server pixel format");
        if(!this->sendClientSetPixelFormat(this->clientPixelF))
            return false;
    }

    //encodings
    if (!this->sendClientSetEncodings(0)) //raw
        return false;

    if (!this->sendClientSetEncodings(-239)) //cursor pseudo
        return false;

    if (!this->sendClientSetEncodings(-223)) //desktopsize pseudo
        return false;



    //qDebug("   emiting resize at %d x %d",rfbSID.fbWidth,rfbSID.fbHeight);
    //qDebug("name length is %d",rfbSID.fbNameLength);
    //qDebug("server name is %s",QString(vncServerData.mid(24,rfbSID.fbNameLength)).toStdString().c_str());

    return this->sendClientFrameBufferRequestUpdate(0,0,rfbSID.fbWidth,rfbSID.fbHeight,0);

    return true;
}


bool rfbclientcls::sendClientFrameBufferRequestUpdate(int x, int y, int width, int height, int incremental)
{

    unsigned char vncClientData[10];
    char inc = 0x01;
    if (incremental == 0)
    {
        //qDebug("frame request is not incremental");
        inc = 0x00;
    }

    vncClientData[0] = 3;
    vncClientData[1]  = inc; //(incremental) ? 1 : 0;
    vncClientData[2]  = (x      >> 8) & 0xFF;
    vncClientData[3]  = (x      >> 0) & 0xFF;
    vncClientData[4]  = (y      >> 8) & 0xFF;
    vncClientData[5]  = (y      >> 0) & 0xFF;
    vncClientData[6]  = (width  >> 8) & 0xFF;
    vncClientData[7]  = (width  >> 0) & 0xFF;
    vncClientData[8]  = (height >> 8) & 0xFF;
    vncClientData[9]  = (height >> 0) & 0xFF;


    if (this->writeToHost(vncClientData,10) != 10)
    {
        qDebug("   fail to write request update");
        return false;
    }

    qDebug("   success request update");
    return true;
}

bool rfbclientcls::sendClientSetEncodings(qint32 encID)
{

    SET_ENCODING_STRUCT enc;
    qint32 encType;


    enc.msgType = 2;
    enc.padding = 2;
    enc.numOfEncodings = 1; //numofencoding in LE
    enc.numOfEncodings = swap16(enc.numOfEncodings); //numofencoding in BE
    enc.encoding = swap32(encID);




    if (this->writeToHost((unsigned char*)&enc,8) != 8)
    {
        qDebug("   fail to set encoding");
        return false;
    }

    return true;

    //send raw encoding
    encType = 0;
    encType = swap32(encType);
    if (this->writeToHost((unsigned char*)&encType,4) != 4)
    {
        qDebug("   fail to set encoding");
        return false;
    }

    //send desktop resize
    encType = -223;
    encType = swap32(encType);
    if (this->writeToHost((unsigned char*)&encType,4) != 4)
    {
        qDebug("   fail to set encoding");
        return false;
    }

    return true;


}

bool rfbclientcls::sendClientSetPixelFormat(pixelFormatStruct pixel)
{
    unsigned char vncClientData[20];

    vncClientData[0] = 0; //message type
    vncClientData[1] = 0x00;
    vncClientData[2] = 0x00;
    vncClientData[3] = 0x00;
    vncClientData[4] = pixel.bpp;
    vncClientData[5] = pixel.depth;
    vncClientData[6] = pixel.big_endian_flag;
    vncClientData[7] = pixel.true_color_flag;
    vncClientData[8] = (pixel.red_max >> 8) & 0xFF;
    vncClientData[9] = (pixel.red_max >> 0) & 0xFF;
    vncClientData[10] = (pixel.green_max >> 8) & 0xFF;
    vncClientData[11] = (pixel.green_max >> 0) & 0xFF;
    vncClientData[12] = (pixel.blue_max >> 8) & 0xFF;
    vncClientData[13] = (pixel.blue_max >> 0) & 0xFF;
    vncClientData[14] = pixel.red_shift;
    vncClientData[15] = pixel.green_shift;
    vncClientData[16] = pixel.blue_shift;
    vncClientData[17] = 0x00;
    vncClientData[18] = 0x00;
    vncClientData[19] = 0x00;

    if (this->writeToHost(vncClientData,20) != 20)
    {
        qDebug("   fail to set pixel format");
        return false;
    }

    return true;
}

QPixmap rfbclientcls::getVNCPixmap()
{
    return QPixmap::fromImage(this->VNCIMAGE);
}


quint16 rfbclientcls::swap16(quint16 src)
{
    quint16 tgt;
    char *tgtX = (char *)&tgt;
    char *srcX = (char *)&src;
    tgtX[0] = srcX[1];
    tgtX[1] = srcX[0];

    return tgt;
}

qint16 rfbclientcls::swap16(qint16 src)
{
    qint16 tgt;
    char *tgtX = (char *)&tgt;
    char *srcX = (char *)&src;
    tgtX[0] = srcX[1];
    tgtX[1] = srcX[0];

    return tgt;
}

quint32 rfbclientcls::swap32(quint32 src)
{
    quint32 tgt;
    char *tgtX = (char *)&tgt;
    char *srcX = (char *)&src;

    tgtX[0] = srcX[3];
    tgtX[1] = srcX[2];
    tgtX[2] = srcX[1];
    tgtX[3] = srcX[0];

    return tgt;
}

qint32 rfbclientcls::swap32(qint32 src)
{
    qint32 tgt;
    char *tgtX = (char *)&tgt;
    char *srcX = (char *)&src;

    tgtX[0] = srcX[3];
    tgtX[1] = srcX[2];
    tgtX[2] = srcX[1];
    tgtX[3] = srcX[0];

    return tgt;
}

qint64 rfbclientcls::writeToHost(unsigned char *src, qint64 size)
{
    qint64 rs = 0;

    switch (this->socketMode)
    {
    case 0:
        rs = this->vncClientTCP.write((char*)src,size);
        if (!this->vncClientTCP.waitForBytesWritten())
            return 0;
        break;

    case 1:
        rs = this->vncClientIPC.write((char*)src,size);
        if (!this->vncClientIPC.waitForBytesWritten())
            return 0;
        break;
    }

    return rs;
}

qint64 rfbclientcls::writeToHostNonBlock(unsigned char *src, qint64 size)
{

    qint64 rs = 0;

    switch (this->socketMode)
    {
    case 0:
        if (this->vncClientTCP.state() == QTcpSocket::ConnectedState)
                rs = this->vncClientTCP.write((char*)src,size);
        break;

    case 1:
        if (this->vncClientIPC.state() == QLocalSocket::ConnectedState)
                rs = this->vncClientIPC.write((char*)src,size);
        break;
    }

    return rs;

}

qint64 rfbclientcls::readFromHost(unsigned char *tgt, qint64 size)
{

    switch (this->socketMode){

    case 0:
        if (this->vncClientTCP.state() == QTcpSocket::ConnectedState)
        {
            while(1){

                QCoreApplication::processEvents();
                if (this->vncClientTCP.bytesAvailable() == 0)
                    this->vncClientTCP.waitForReadyRead(); // wait for data to arrive

                if (this->vncClientTCP.bytesAvailable() >= size){
                    return this->vncClientTCP.read((char*)tgt,size);
                }
            }
        }
        break;

    case 1:
        if (this->vncClientIPC.state() == QLocalSocket::ConnectedState)
        {
            while(1){

                QCoreApplication::processEvents();
                if (this->vncClientIPC.bytesAvailable() == 0)
                    this->vncClientIPC.waitForReadyRead(); // wait for data to arrive

                if (this->vncClientIPC.bytesAvailable() >= size){
                    return this->vncClientIPC.read((char*)tgt,size);
                }
            }
        }
        break;
    }

    return 0; // disconnected state



    /*switch (this->socketMode)
    {
    case 0:
        if (this->vncClientTCP.state() == QTcpSocket::ConnectedState)
        {
                bSize = this->vncClientTCP.bytesAvailable();
        }
        else
                bSize = 0;
        break;

    case 1:
        if (this->vncClientIPC.state() == QLocalSocket::ConnectedState)
            bSize = this->vncClientIPC.bytesAvailable();
        else
            bSize = 0;
        break;
    }

    if (!wait)
    {
        if ((bSize == 0) || size > bSize)
        {
            return 0; //no data
        }

    }

    //socket will wait until data arrived;
    //if there is data
    while(1)
    {
        QCoreApplication::processEvents();
        switch (this->socketMode)
        {
        case 0:
            if (this->vncClientTCP.state() == QTcpSocket::ConnectedState)
                    bSize = this->vncClientTCP.bytesAvailable();
            else
                    bSize = 0;
            break;

        case 1:
            if (this->vncClientIPC.state() == QLocalSocket::ConnectedState)
                bSize = this->vncClientIPC.bytesAvailable();
            else
                bSize = 0;
            break;
        }
        if (bSize >=size)
            break;
    }

    switch (this->socketMode)
    {
    case 0:
        if (this->vncClientIPC.state() == QLocalSocket::ConnectedState)
            this->vncClientTCP.read((char*)tgt,size);
        else
            bSize = 0;
        break;

    case 1:
        if (this->vncClientIPC.state() == QLocalSocket::ConnectedState)
            this->vncClientIPC.read((char*)tgt,size);
        else
            bSize = 0;
        break;
    }

    return size;*/

}


void rfbclientcls::sendServerPointerEvent(quint16 x, quint16 y, quint8 buttonMask)
{
    this->sendClientPointerEvent(x,y,buttonMask);
}

void rfbclientcls::sendClientPointerEvent(quint16 x, quint16 y, quint8 buttonMask)
{
    SENT_POINTER_EVENT pointer;

    if (!this->serverConnected)
        return;

    pointer.msgType = 0x05;
    pointer.x = swap16(x);
    pointer.y = swap16(y);
    pointer.buttonMask = buttonMask;

    this->writeToHostNonBlock((unsigned char*)&pointer,6);
    //this->writeToHost((unsigned char*)&pointer,6);

}

void rfbclientcls::sendServerKeyEvent(quint32 pKey, quint8 press, bool modifier)
{
    this->sendClientKeyEvent(pKey,press,modifier);
}

void rfbclientcls::sendClientKeyEvent(quint32 pKey, quint8 press, bool modifier)
{
    SENT_KEY_EVENT clientKey;

    if (!this->serverConnected)
        return;

    clientKey.msgType = 0x04;
    clientKey.padding = 0;
    clientKey.press = press;

    clientKey.keyValue = this->translateRfbKey(pKey,modifier);
    clientKey.keyValue = swap32(clientKey.keyValue);

    this->writeToHostNonBlock((unsigned char*)&clientKey,8);
}

quint32 rfbclientcls::translateRfbKey(quint32 inkey,bool modifier)
{
    quint32 k = 5000;

    switch (inkey)
    {

        case Qt::Key_Backspace: k = XK_BackSpace; break;
        case Qt::Key_Tab: k = XK_Tab;break;
        case Qt::Key_Clear: k = XK_Clear; break;
        case Qt::Key_Return: k = XK_Return; break;
        case Qt::Key_Pause: k = XK_Pause; break;
        case Qt::Key_Escape: k = XK_Escape; break;
        case Qt::Key_Space: k = XK_space; break;
        case Qt::Key_Delete: k = XK_Delete; break;
        case Qt::Key_Period: k = XK_period; break;

        //special keyboard char
        case Qt::Key_Exclam: k = XK_exclam;break; //!
        case Qt::Key_QuoteDbl: k = XK_quotedbl;break; //?
        case Qt::Key_NumberSign: k = XK_numbersign;break; //#
        case Qt::Key_Percent: k = XK_percent;break; //%
        case Qt::Key_Dollar: k = XK_dollar;break;   //$
        case Qt::Key_Ampersand: k = XK_ampersand;break; //&
        case Qt::Key_Apostrophe: k = XK_apostrophe;break;//!
        case Qt::Key_ParenLeft: k = XK_parenleft;break;
        case Qt::Key_ParenRight: k = XK_parenright;break;

        case Qt::Key_Slash: k = XK_slash; break;    ///
        case Qt::Key_Asterisk: k = XK_asterisk; break;  //*
        case Qt::Key_Minus: k = XK_minus; break;    //-
        case Qt::Key_Plus: k = XK_plus; break;  //+
        case Qt::Key_Enter: k = XK_Return; break;   //
        case Qt::Key_Equal: k = XK_equal; break;    //=

        case Qt::Key_Colon: k = XK_colon;break;
        case Qt::Key_Semicolon: k = XK_semicolon; break;
        case Qt::Key_Greater: k = XK_greater; break;
        case Qt::Key_Question: k = XK_question; break;
        case Qt::Key_At: k = XK_at; break;

        case Qt::Key_BracketLeft: k = XK_bracketleft; break;
        case Qt::Key_Backslash: k = XK_backslash;break;
        case Qt::Key_BracketRight: k = XK_bracketright;break;
        case Qt::Key_AsciiCircum: k = XK_asciicircum;break;
        case Qt::Key_Underscore: k = XK_underscore;break;
        case Qt::Key_QuoteLeft: k = XK_quoteleft;break;
        case Qt::Key_BraceLeft: k = XK_braceleft;break;
        case Qt::Key_Bar: k = XK_bar; break;
        case Qt::Key_BraceRight: k = XK_braceright;break;
        case Qt::Key_AsciiTilde: k = XK_asciitilde;break;
        case Qt::Key_nobreakspace: k = XK_nobreakspace;break;
        case Qt::Key_exclamdown: k = XK_exclamdown;break;
        case Qt::Key_cent: k = XK_cent;break;
        case Qt::Key_sterling: k = XK_sterling;break;
        case Qt::Key_currency: k = XK_currency;break;
        case Qt::Key_yen: k = XK_yen;break;
        case Qt::Key_brokenbar: k = XK_brokenbar;break;
        case Qt::Key_section: k = XK_section;break;
        case Qt::Key_diaeresis: k = XK_diaeresis;break;
        case Qt::Key_copyright: k = XK_copyright; break;
        case Qt::Key_ordfeminine: k = XK_ordfeminine; break;
        case Qt::Key_guillemotleft: k = XK_guillemotleft; break;
        case Qt::Key_guillemotright: k = XK_guillemotright; break;
        case Qt::Key_notsign: k = XK_notsign; break;
        case Qt::Key_hyphen: k = XK_hyphen; break;
        case  Qt::Key_registered: k = XK_registered; break;

        case Qt::Key_Up: k = XK_Up; break;
        case Qt::Key_Down: k = XK_Down; break;
        case Qt::Key_Right: k = XK_Right; break;
        case Qt::Key_Left: k = XK_Left; break;
        case Qt::Key_Insert: k = XK_Insert; break;
        case Qt::Key_Home: k = XK_Home; break;
        case Qt::Key_End: k = XK_End; break;
        case Qt::Key_PageUp: k = XK_Page_Up; break;
        case Qt::Key_PageDown: k = XK_Page_Down; break;
        case Qt::Key_F1: k = XK_F1; break;
        case Qt::Key_F2: k = XK_F2; break;
        case Qt::Key_F3: k = XK_F3; break;
        case Qt::Key_F4: k = XK_F4; break;
        case Qt::Key_F5: k = XK_F5; break;
        case Qt::Key_F6: k = XK_F6; break;
        case Qt::Key_F7: k = XK_F7; break;
        case Qt::Key_F8: k = XK_F8; break;
        case Qt::Key_F9: k = XK_F9; break;
        case Qt::Key_F10: k = XK_F10; break;
        case Qt::Key_F11: k = XK_F11; break;
        case Qt::Key_F12: k =  XK_F12; break;
        case Qt::Key_F13: k = XK_F13; break;
        case Qt::Key_F14: k = XK_F14; break;
        case Qt::Key_F15: k = XK_F15; break;
        case Qt::Key_F16: k = XK_F16; break;
        case Qt::Key_F17: k = XK_F17; break;
        case Qt::Key_F18: k = XK_F18; break;
        case Qt::Key_F19: k = XK_F19; break;
        case Qt::Key_F20: k = XK_F20; break;
        case Qt::Key_F21: k = XK_F21; break;
        case Qt::Key_F22: k = XK_F22; break;
        case Qt::Key_F23: k = XK_F23; break;
        case Qt::Key_F24: k = XK_F24; break;
        case Qt::Key_F25: k = XK_F25; break;
        case Qt::Key_F26: k = XK_F26; break;
        case Qt::Key_F27: k = XK_F27; break;
        case Qt::Key_F28: k = XK_F28; break;
        case Qt::Key_F29: k = XK_F29; break;
        case Qt::Key_F30: k = XK_F30; break;
        case Qt::Key_F31: k = XK_F31; break;
        case Qt::Key_F32: k = XK_F32; break;
        case Qt::Key_F33: k = XK_F33; break;
        case Qt::Key_F34: k = XK_F34; break;
        case Qt::Key_F35: k = XK_F35; break;
        case Qt::Key_NumLock: k = XK_Num_Lock; break;
        case Qt::Key_CapsLock: k = XK_Caps_Lock; break;
        case Qt::Key_ScrollLock: k = XK_Scroll_Lock; break;
        case Qt::Key_Shift: k = XK_Shift_R; break; //k = XK_Shift_L; break;
        case Qt::Key_Control: k = XK_Control_R; break;// k = XK_Control_L; break;
        case Qt::Key_Alt: k = XK_Alt_R; break;//k = XK_Alt_L; break;
        case Qt::Key_Meta: k = XK_Meta_R; break;//k = XK_Meta_L; break;*/

        case Qt::Key_Super_L: k = XK_Super_L; break;		/* left "windows" key */
        case Qt::Key_Super_R: k = XK_Super_R; break;		/* right "windows" key */

        case Qt::Key_Mode_switch: k = XK_Mode_switch; break;
        case Qt::Key_Help: k = XK_Help; break;
        case Qt::Key_Print: k = XK_Print; break;
        case Qt::Key_SysReq: k = XK_Sys_Req; break;
        case Qt::Key_0: k = XK_0;break;
        case Qt::Key_1: k = XK_1;break;
        case Qt::Key_2: k = XK_2;break;
        case Qt::Key_3: k = XK_3;break;
        case Qt::Key_4: k = XK_4;break;
        case Qt::Key_5: k = XK_5;break;
        case Qt::Key_6: k = XK_6;break;
        case Qt::Key_7: k = XK_7;break;
        case Qt::Key_8: k = XK_8;break;
        case Qt::Key_9: k = XK_9;break;
    }

    if (k == 5000)
    {

        if (!modifier)
        {
            switch (inkey)
            {
                case Qt::Key_A: k = XK_a;break;
                case Qt::Key_B: k = XK_b;break;
                case Qt::Key_C: k = XK_c;break;
                case Qt::Key_D: k = XK_d;break;
                case Qt::Key_E: k = XK_e;break;
                case Qt::Key_F: k = XK_f;break;
                case Qt::Key_G: k = XK_g;break;
                case Qt::Key_H: k = XK_h;break;
                case Qt::Key_I: k = XK_i;break;
                case Qt::Key_J: k = XK_j;break;
                case Qt::Key_K: k = XK_k;break;
                case Qt::Key_L: k = XK_l;break;
                case Qt::Key_M: k = XK_m;break;
                case Qt::Key_N: k = XK_n;break;
                case Qt::Key_O: k = XK_o;break;
                case Qt::Key_P: k = XK_p;break;
                case Qt::Key_Q: k = XK_q;break;
                case Qt::Key_R: k = XK_r;break;
                case Qt::Key_S: k = XK_s;break;
                case Qt::Key_T: k = XK_t;break;
                case Qt::Key_U: k = XK_u;break;
                case Qt::Key_V: k = XK_v;break;
                case Qt::Key_W: k = XK_w;break;
                case Qt::Key_X: k = XK_x;break;
                case Qt::Key_Y: k = XK_y;break;
                case Qt::Key_Z: k = XK_z;break;
            }
        }
        else
        {
            switch (inkey)
            {
                case Qt::Key_A: k = XK_A;break;
                case Qt::Key_B: k = XK_B;break;
                case Qt::Key_C: k = XK_C;break;
                case Qt::Key_D: k = XK_D;break;
                case Qt::Key_E: k = XK_E;break;
                case Qt::Key_F: k = XK_F;break;
                case Qt::Key_G: k = XK_G;break;
                case Qt::Key_H: k = XK_H;break;
                case Qt::Key_I: k = XK_I;break;
                case Qt::Key_J: k = XK_J;break;
                case Qt::Key_K: k = XK_K;break;
                case Qt::Key_L: k = XK_L;break;
                case Qt::Key_M: k = XK_M;break;
                case Qt::Key_N: k = XK_N;break;
                case Qt::Key_O: k = XK_O;break;
                case Qt::Key_P: k = XK_P;break;
                case Qt::Key_Q: k = XK_Q;break;
                case Qt::Key_R: k = XK_R;break;
                case Qt::Key_S: k = XK_S;break;
                case Qt::Key_T: k = XK_T;break;
                case Qt::Key_U: k = XK_U;break;
                case Qt::Key_V: k = XK_V;break;
                case Qt::Key_W: k = XK_W;break;
                case Qt::Key_X: k = XK_X;break;
                case Qt::Key_Y: k = XK_Y;break;
                case Qt::Key_Z: k = XK_Z;break;
            }
        }
    }

    return k;

}

void rfbclientcls::vncIPCSockErrorSlot(QLocalSocket::LocalSocketError)
{

    //QMessageBox::critical(0,"RFB Error",this->vncClientIPC->errorString());
    this->vncClientIPC.abort();
}

void rfbclientcls::vncTCPSockErrorSlot(QAbstractSocket::SocketError)
{
    //QMessageBox::critical(0,"RFB Error",this->vncClientTCP->errorString());
    this->vncClientTCP.abort();
}

