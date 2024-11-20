/*
 *  ARIA Sensing 2024
 *  Author: Cacciatori Alessio
 *  This program is licensed under LGPLv3.0
 */
#include "antenna.h"
#include "math.h"
#include "ovl.h"

#include "parse.h"
#include "qdebug.h"
#include "qvariant.h"
#include <complex>
#include <QtSql/QSqlDatabase>
#include <QtSql/QSqlQuery>
#include <QFile>
#include <QDataStream>
#include <QTextStream>
#include <ariautils.h>
#include <QFileInfo>
#include <QtXml>
#include <octaveinterface.h>
#include <ariautils.h>

#define  XML_ANTENNAFILE_MAJOR  0
#define  XML_ANTENNAFILE_MINOR  1
#define  XML_ANTENNAFILE_SUBVER 0

extern class octaveInterface         *interfaceData;   // Convenient copy

NDArray dB_field(ComplexNDArray input)
{
    NDArray out(input.dims());
    int numel = input.numel();
    for (int n=0; n < numel; n++)
    {
        double r = input(n).real();
        double i = input(n).imag();
        out(n) = 10.0*log10(r*r+i*i);
    }
    return out;
}


NDArray dB_field(NDArray input, bool db20)
{
    NDArray out(input.dims());
    int numel = input.numel();
    double k = db20?20.0:10.0;
    for (int n=0; n < numel; n++)
        out(n) = k*log10(fabs(input(n)));
    return out;
}

//-------------------------------------------------------
antenna::antenna(projectItem* parent) : projectItem("newantenna", DT_ANTENNA, parent)
{
    freqs = octave_value(linspace(5e9, 12e9, (7e9+100e6)/100e6)).vector_value();
    double dangle = 5.0*M_PI/180;
    double az_max = 360*M_PI/180;
    double zen_max = 180*M_PI/180;
    azimuth=octave_value(linspace(0,az_max, (az_max+dangle)/dangle)).vector_value();
    zenith =octave_value(linspace(0,zen_max, (zen_max+dangle)/dangle)).vector_value();

    int naz     = azimuth.numel();
    int nzen    = zenith.numel();
    int nfreqs  = freqs.numel();
    //octave_idx_type dims[3] = {naz,nzen,nfreqs};

    et = ComplexNDArray(dim_vector(naz,nzen,nfreqs),Complex(1.0,0.0));
    ep = et; //ComplexNDArray(dim_vector(dims),Complex(0.0,0.0));
    gain = NDArray(dim_vector(naz,nzen,nfreqs),0);
    s11 = ComplexNDArray(dim_vector(1,nfreqs),Complex(0.0,0.0));
    s11_freqs = freqs;
    bModified=true;
}
//-------------------------------------------------------


antenna::antenna(QString filename,projectItem* parent) : antenna(parent)
{
    set_filename(filename);
    if (load(filename))
        bModified = false;
}

void antenna::set_filename(QString filename)
{
    _filename = filename;
    antenna_file.setFileName(filename);
    if (filename.isEmpty())
        set_name("[noname]");
    else
        set_name(QFileInfo(filename).fileName());
}
//-------------------------------------------------------
antenna::antenna(antenna& ant2) : antenna()
{
    set_filename(ant2.get_full_filepath());
    projectItem::operator=(ant2);
    setFields(ant2.azimuth, ant2.zenith, ant2.freqs, ant2.et, ant2. ep);
    s11  = ant2.s11;
    bModified = ant2.bModified;
    gainCalculated = ant2.gainCalculated;
}

//-------------------------------------------------------
antenna::antenna(const antenna& ant2) : antenna()
{
    set_filename(ant2._filename);
    projectItem::operator=(ant2);

    setFields(ant2.azimuth, ant2.zenith, ant2.freqs, ant2.et, ant2. ep);
    s11  = ant2.s11;
    bModified = ant2.bModified;
    gainCalculated = ant2.gainCalculated;
}

//-------------------------------------------------------
antenna::~antenna()
{
    if (antenna_file.isOpen())
        antenna_file.close();
}
//-------------------------------------------------------
antenna&     antenna::operator = (antenna& ant2)
{
   projectItem::operator=(ant2);
    setFields(ant2.azimuth, ant2.zenith, ant2.freqs, ant2.et, ant2. ep);
    s11  = ant2.s11;
    set_filename(ant2._filename);
    bModified = ant2.bModified;
    gainCalculated = ant2.gainCalculated;
    return *this;
}
//-------------------------------------------------------
antenna&     antenna::operator = (const antenna& ant2)
{
    projectItem::operator=(ant2);
    setFields(ant2.azimuth, ant2.zenith, ant2.freqs, ant2.et, ant2. ep);
    s11  = ant2.s11;
    set_filename(ant2._filename);
    bModified = ant2.bModified;
    gainCalculated = ant2.gainCalculated;
    return *this;
}

//-------------------------------------------------------
// Set the fields according to the input data
// The data is uniformly resampled
//-------------------------------------------------------
void antenna::setFields(const octave_value& azsupport,const octave_value& zensupport,
                        const octave_value& freqsupport, const octave_value& EtField,
                        const octave_value& EpField)
{
    gainCalculated = false;
    freqs   = freqsupport.array_value();
    azimuth = azsupport.array_value();
    zenith  = zensupport.array_value();
    et      = EtField.complex_array_value();
    ep      = EpField.complex_array_value();

    calcGain();
    bModified = true;
}
//-------------------------------------------------------
double antenna::dS(double azimuth_angle,double zenith_angle,  double r)
{
    double da = azimuth.numel()>1? azimuth(1)-azimuth(0) : 1.0;
    double dz = zenith.numel()>1? zenith(1)-zenith(0) : 1.0;
    if (abs(zenith_angle)<1e-3) zenith_angle = dz/2;
    return (r*r)*sin(zenith_angle)*da*dz;
}
//-------------------------------------------------------
double  antenna::getGainInterp(double az, double zen, double f)
{
    calcGain();
    int nf0=-1;
    int nf1=-1;
    for (int fi = 0; fi < freqs.numel(); fi++)
    {
        if (freqs(fi)<=f)
            nf0 = fi;
        else
        {
            nf1 = fi;
            break;
        }
    }

    if ((nf0<0)&&(nf1<0))
        return -1;

    if (nf0<0) nf0=nf1;
    if (nf1<0) nf1=nf0;
    // Search prev and succ azimuth
    int na0 = -1;
    int na1 = -1;

    for (int ai=0; ai < azimuth.numel(); ai++)
    {
        if (azimuth(ai)<=az)
            na0 = ai;
        else
        {na1=ai; break;}
    }

    if ((na0<0)&&(na1<0))
        return -1;

    if (na0<0) na0=na1;
    if (na1<0) na1=na0;

    // Search prev and succ
    int nz0 = -1;
    int nz1 = -1;

    for (int zi=0; zi < zenith.numel(); zi++)
    {
        if (zenith(zi)<=zen)
            nz0 = zi;
        else
        { nz1 = zi; break;}
    }

    if ((nz0<0)&&(nz1<0))
        return -1;

    if (nz0<0) nz0=nz1;
    if (nz1<0) nz1=nz0;

    double da = azimuth(na1)-azimuth(na0);
    double df = freqs(nf1)-freqs(nf0);
    double dz = zenith(nz1)-zenith(nz0);


    double dGain_df = abs(df) > 1e-3 ? (f-freqs(nf0)) * (gain(na0,nz0,nf1)-gain(na0,nz0,nf0))/df : 0;
    double dGain_da = abs(da) > 1e-6 ? (az-azimuth(na0))* (gain(na1,nz0,nf0)-gain(na0,nz0,nf0))/da : 0;
    double dGain_dz = abs(dz) > 1e-6 ? (zen - zenith(nz0))* (gain(na0,nz1,nf0)-gain(na0,nz0,nf0))/dz : 0;

    double Gain =  gain(na0,nz0,nf0) + dGain_df  + dGain_da + dGain_dz;

    return Gain;
}
//-------------------------------------------------------
bool    antenna::getEtEpInterp(double az, double zen, double f, Complex& etval, Complex& epval)
{
    int nf0=-1;
    int nf1=-1;
    for (int fi = 0; fi < freqs.numel(); fi++)
    {
        if ((nf1<0)&&(freqs(fi)>f))
            nf1 = fi;

        if (freqs(fi)<=f)
            nf0 = fi;
    }

    if ((nf0<0)&&(nf1<0))
        return false;

    if (nf0<0) nf0=nf1;
    if (nf1<0) nf1=nf0;
    // Search prev and succ azimuth
    int na0 = -1;
    int na1 = -1;

    for (int ai=0; ai < azimuth.numel(); ai++)
    {
        if ((na1<0)&&(azimuth(ai)>az))
            na1 = ai;

        if (azimuth(ai)<=az)
            na0 = ai;
    }

    if ((na0<0)&&(na1<0))
        return false;

    if (na0<0) na0=na1;
    if (na1<0) na1=na0;

    // Search prev and succ
    int nz0 = -1;
    int nz1 = -1;

    for (int zi=0; zi < zenith.numel(); zi++)
    {
        if ((na1<0)&&(zenith(zi)>zen))
            nz1 = zi;

        if (zenith(zi)<=zen)
            nz0 = zi;
    }

    if ((na0<0)&&(na1<0))
        return false;

    if (nz0<0) nz0=nz1;
    if (nz1<0) nz1=nz0;

    double da = azimuth(na1)-azimuth(na0);
    double df = freqs(nf1)-freqs(nf0);
    double dz = zenith(nz1)-zenith(nz0);


    Complex dEt_df = (et(na0,nz0,nf1)-et(na0,nz0,nf0))/df;
    Complex dEt_da = (et(na1,nz0,nf0)-et(na0,nz0,nf0))/da;
    Complex dEt_dz = (et(na0,nz1,nf0)-et(na0,nz0,nf0))/dz;

    Complex dEp_df = (ep(na0,nz0,nf1)-ep(na0,nz0,nf0))/df;
    Complex dEp_da = (ep(na1,nz0,nf0)-ep(na0,nz0,nf0))/da;
    Complex dEp_dz = (ep(na0,nz1,nf0)-ep(na0,nz0,nf0))/dz;

    etval =  et(na0,nz0,nf0) + dEt_df * (f-freqs(nf0)) + dEt_da * (az-azimuth(na0)) + dEt_dz * (zen - zenith(nz0));
    epval =  ep(na0,nz0,nf0) + dEp_df * (f-freqs(nf0)) + dEp_da * (az-azimuth(na0)) + dEp_dz * (zen - zenith(nz0));

    return true;
}
//-------------------------------------------------------
double antenna::calculateRadiatedPwrAtFreq(int freqid)
{
    double tPwr = 0.0;
    double da = azimuth.numel()>1?(azimuth(2)-azimuth(1)):1.0;
    double dz = zenith.numel()>1?(zenith(2)-zenith(1)):1.0;

    for (int nz = 0; nz < zenith.numel(); nz++)
    {
        double zen = zenith(nz);
        if (abs(zen)<1e-3)
            zen = dz/2;

        double ds = sin(zen)*da*dz;
        for (int na = 0; na < azimuth.numel(); na++)
            tPwr += ds * calculatePowerFlow(na,nz,freqid);
    }
    return tPwr;
}

//-------------------------------------------------------
double antenna::calculateRadiatedPwrAtFreqInterp(double freq)
{
    double tPwr = 0.0;
    int nf0=-1;
    int nf1=-1;
    for (int fi = 0; fi < freqs.numel(); fi++)
    {
        if ((nf1<0)&&(freqs(fi)>freq))
            nf1 = fi;

        if (freqs(fi)<=freq)
            nf0 = fi;
    }

    if ((nf0<0)&&(nf1<0))
        return 1e99;

    if (nf0<0) nf0=nf1;
    if (nf1<0) nf1=nf0;

    double kint = nf1==nf0? 0.0 : (freq-freqs(nf0))/(freqs(nf1)-freqs(nf0));
    double da = azimuth.numel()>1?(azimuth(2)-azimuth(1)):1.0;
    double dz = zenith.numel()>1?(zenith(2)-zenith(1)):1.0;

    for (int nz = 0; nz < zenith.numel(); nz++)
    {
        double zen = zenith(nz);
        if (abs(zen)<1e-3) zen = dz/2;

        double ds = sin(zen)*da*dz;
        for (int na = 0; na < azimuth.numel(); na++)
        {
            Complex et0 = et(na,nz,nf0);
            Complex ep0 = ep(na,nz,nf0);
            Complex delta_et = et(na,nz,nf1) - et0;
            Complex delta_ep = ep(na,nz,nf1) - ep0;
            Complex eti = et0 + kint * delta_et;
            Complex epi = ep0 + kint * delta_ep;
            tPwr += ONE_OVERETA0 * ds * ((eti*conj(eti)).real() + (epi*conj(epi)).real());
        }
    }
    return tPwr;
}
//-------------------------------------------------------
double antenna::calculatePowerFlowInterp(double a, double z, double f)
{
    Complex eps, ets; // Fields at surface points
    if (!getEtEpInterp(a,z,f,ets,eps))
        return -1.0; // Set negative power flow (impossible) as error condition
    return ONE_OVERETA0 * ((ets*conj(ets)).real() + (eps*conj(eps)).real());
}
//-------------------------------------------------------
double antenna::calculatePowerFlow(int aid, int zid, int fid)
{
    if ((fid <0)||(fid>=freqs.numel())) return -1.0;
    if ((aid <0)||(aid>=azimuth.numel())) return -1.0;
    if ((zid <0)||(zid>=zenith.numel())) return -1.0;
    Complex ets = et(aid,zid,fid);
    Complex eps = ep(aid,zid,fid);
    return ONE_OVERETA0 * ((ets*conj(ets)).real() + (eps*conj(eps)).real());
}
//-------------------------------------------------------
void antenna::calcGain()
{
    if (gainCalculated)
        return;

    gain.resize(dim_vector(azimuth.numel(), zenith.numel(), freqs.numel()),0.0);

    //double da = azimuth.numel()>1? azimuth(1)-azimuth(0) : 1.0;
    // double dz = zenith.numel()>1? zenith(1)-zenith(0) : 1.0;
    for (int nf = 0; nf < freqs.numel(); nf++)
    {
        double tPwr = calculateRadiatedPwrAtFreq(nf)/(4.0*M_PI);

        if (tPwr < 0)
            tPwr = 1e10;

        for (int nz = 0; nz < zenith.numel(); nz++)
        {
          //  double zen = zenith(nz);
          //  if (abs(zen)<1e-3) zen = dz/2;

          //  double ds = sin(zen)*da*dz;

            for (int na = 0; na < azimuth.numel(); na++)
            {
                double g = calculatePowerFlow(na,nz,nf);
                gain(na,nz,nf) = g/tPwr;
            }
        }     
    }

    gainCalculated=true;
}
//-------------------------------------------------------
double antenna::getFreqMin()
{
    return freqs.min()(0);
}
double antenna::getFreqMax()
{
    return freqs.max()(0);
}
//-------------------------------------------------------
NDArray antenna::getAzimuth()  {return azimuth;}
//-------------------------------------------------------
NDArray antenna::getZenith()   {return zenith;}
//-------------------------------------------------------
NDArray antenna::getFreqs()    {return freqs;}
//-------------------------------------------------------
NDArray antenna::getGain(int freqid)
{
    calcGain();
    if ((freqid <0 )||(freqid>=freqs.numel()))
        return NDArray();
    return gain.page(freqid);
}
//-------------------------------------------------------
ComplexNDArray antenna::getEp(int freqid)
{

    if ((freqid <0 )||(freqid>=freqs.numel()))
        return ComplexNDArray();

    return ep.page(freqid);
}
//-------------------------------------------------------
ComplexNDArray antenna::getEt(int freqid)
{
    if ((freqid <0 )||(freqid>=freqs.numel()))
        return ComplexNDArray();
    return et.page(freqid);
}


//-------------------------------------------------------
void antenna::insertEpEtFreq(double freq, ComplexNDArray Ep, ComplexNDArray Et)
{
    // Index < 0: resulting array must be
    // freqs .. value
    int newFreqSize = freqs.numel()+1;
    ComplexNDArray newEpArray(dim_vector(azimuth.numel(),zenith.numel(),newFreqSize), 0);
    ComplexNDArray newEtArray(dim_vector(azimuth.numel(),zenith.numel(),newFreqSize), 0);
    NDArray newfarray(dim_vector(1,newFreqSize), 0);
    // This is the index where we need to insert value

    Array<octave_idx_type> dest(dim_vector(3,1));
    dest(0)=0;
    dest(1)=0;

    Array<octave_idx_type> source(dim_vector(3,1));

    int positionInsert = -1;

    for (int fi=0; fi < freqs.numel(); fi++)
    {
        if (freqs(fi)>freq)
        {
            positionInsert = fi;
            break;
        }
    }

    int indexSource = 0;
    if (positionInsert<0) positionInsert = freqs.numel();
    for (int f=0; f < newFreqSize; f++)
    {
        dest(2) = f;
        if (f==positionInsert)
        {
            for (int zi = 0 ; zi < zenith.numel(); zi++)
                for (int ai = 0; ai < azimuth.numel(); ai++)
                {
                    newEpArray(ai,zi,f)=Ep(ai,zi);
                    newEtArray(ai,zi,f)=Et(ai,zi);
                }
            newfarray(positionInsert)=freq;
        }
        else
        {
            for (int zi = 0 ; zi < zenith.numel(); zi++)
                for (int ai = 0; ai < azimuth.numel(); ai++)
                {
                    newEpArray(ai,zi,f)=ep(ai,zi,indexSource);
                    newEtArray(ai,zi,f)=et(ai,zi,indexSource);
                }

            newfarray(f)=freqs(indexSource);
            indexSource++;
        }

    }
//    freqs = newfarray;
    ep = newEpArray;
    et = newEtArray;
    freqs = newfarray;
    gainCalculated = false;
}

//-------------------------------------------------------
bool antenna::insertFreqs(const QVector<double>& newfreq, ComplexNDArray Et, ComplexNDArray Ep)
{        
    if ((zenith.numel()==0)||(azimuth.numel()==0))
    {
        dim_vector dims = Et.dims();
        if ((dims(0)!=azimuth.numel())||(dims(1)!=zenith.numel()))
            return false;
        dims = Ep.dims();
        if ((dims(0)!=azimuth.numel())||(dims(1)!=zenith.numel()))
            return false;
    }

    NDArray freqToAdd;

    for (int fni = 0; fni < newfreq.count(); fni++)
    {
        double f = newfreq[fni];
        // We may have n0 = -1 (current newFreq is the higest)
        // or n0>=0. In the latter, the resulting freq must be
        //
        insertEpEtFreq(f, Ep.page(fni),Et.page(fni));
    }
    return true;
}
//-------------------------------------------------------
bool antenna::loadFarField(const QStringList &filelist)
{
    NDArray old_freqs = freqs;
    freqs.clear(0,0);
    azimuth.clear(0,0);
    zenith.clear(0,0);
    ep.clear();// = ComplexNDArray(dim_vector({0,0}));
    et.clear();// = ComplexNDArray(dim_vector({0,0}));
    gain.clear(0,0);
    s11.clear(0,0);
    QSqlDatabase qdb = QSqlDatabase::addDatabase("QSQLITE");
    for (QStringList::ConstIterator fiter = filelist.begin(); fiter != filelist.end(); fiter++)
    {
        QString filename = (*fiter);
        qdb.setDatabaseName(filename);
        if (!qdb.open())
            return false;

        QSqlQuery q(qdb);
        if (!q.exec("SELECT frequency FROM far_field_patterns"))
        {
            qdb.close();
            return false;
        }

        QVector<double> queryFreq;
        while (q.next())
            queryFreq.append(q.value(0).toDouble());

        if (!q.exec("SELECT theta FROM far_field_samples"))
        {
            qdb.close();
            return false;
        }

        QVector<double> zenList;

        while (q.next())
            zenList.append(q.value(0).toDouble());
        // Remove duplicates zenith values
        std::sort( zenList.begin(), zenList.end() );
        zenList.erase( std::unique(zenList.begin(), zenList.end() ), zenList.end() );

        if (!q.exec("SELECT phi FROM far_field_samples"))
        {
            qdb.close();
            return false;
        }

        QVector<double> azList;
        while (q.next())
            azList.append(q.value(0).toDouble());
        // Remove duplicates zenith values
        std::sort( azList.begin(), azList.end() );
        azList.erase( std::unique(azList.begin(), azList.end() ), azList.end() );

        // We have the size of azimuth and zenith support,
        // Check if these are consistent with existing data
        // otherwise, exit
        if (azimuth.isempty())
        {
            azimuth.resize1(azList.count());
            for (int n=0; n<azList.count(); n++)
                azimuth(n)=azList.at(n);
        }
        else
            if (azimuth.numel()!=azList.count())
                return false;

        if (zenith.isempty())
        {
                zenith.resize1(zenList.count());
                for (int n=0; n<zenList.count(); n++)
                zenith(n)=zenList.at(n);
        }
        else
                if (zenith.numel()!=zenList.count())
                return false;


        if (!q.exec("SELECT Etheta_real,Etheta_imag FROM far_field_samples"))
                return false;

        // Read Etheta

        ComplexNDArray newet(dim_vector(azimuth.numel(),zenith.numel(),queryFreq.count()),Complex(0,0));
        ComplexNDArray newep(dim_vector(azimuth.numel(),zenith.numel(),queryFreq.count()),Complex(0,0));

        int aid = 0;
        int zid = 0;
        while (q.next())
        {
                double r = q.value(0).toDouble();
                double i = q.value(1).toDouble();
                newet(aid++,zid) = Complex(r,i);
                if (aid==azimuth.numel())
                    {aid=0; zid++;}
        }

        if (!q.exec("SELECT Ephi_real,Ephi_imag FROM far_field_samples"))
                return false;

        // Read Ephi
        aid = 0;
        zid = 0;

        while (q.next())
        {
                double r = q.value(0).toDouble();
                double i = q.value(1).toDouble();
                newep(aid++,zid) = Complex(r,i);
                if (aid==azimuth.numel())
                    {aid=0; zid++;}
        }

        if (!insertFreqs(queryFreq,newet,newep))
                return false;

        qdb.close();
    }
    //FFIO Files are handled by SQL
    // Resample s11 according to FFIO freqs?
    // Why not.
    octave_value_list in;
    octave_value_list out;
    in(0)=old_freqs;
    in(1)=s11;
    in(2)=freqs;

    s11 = interp1(old_freqs, s11, freqs);

    bModified=true;
    calcGain();
    return true;
}

//-------------------------------------------------------
// Equivalent to save as
bool antenna::save(QString filename)
{
    if (!filename.isEmpty())
        set_filename(filename);

    return save_xml();
}
//-------------------------------------------------------
bool antenna::load(QString filename)
{

    if (!filename.isEmpty())
        set_filename(filename);

    return load_xml();
}

//-------------------------------------------------------
bool antenna::save_xml()
{
    if (!antenna_file.open(QFile::WriteOnly | QFile::Text ))
    {
        qDebug() << "Already opened or permission issue";
        antenna_file.close();
        return false;
    }
    QTextStream xmlContent(&antenna_file);

    QDomDocument document;

    if (!save_xml(document)) {antenna_file.close(); return false;}


    xmlContent << document.toString();
    antenna_file.close();

    return true;
}

bool antenna::save_xml(QDomDocument& document)
{

    QDomElement root = document.createElement("ARIA_Antenna");

    QVersionNumber xml_ver({XML_ANTENNAFILE_MAJOR,XML_ANTENNAFILE_MINOR,XML_ANTENNAFILE_SUBVER});
    root.setAttribute("xml_dataformat", xml_ver.toString());

    document.appendChild(root);
    QDomElement data_support = document.createElement("data_configuration");
    QDomElement data         = document.createElement("antenna_data");

    root.appendChild(data_support);
    root.appendChild(data);


    QDomElement azimuth_root = document.createElement("azimuth_support");
    azimuth_root.setAttribute("size",QString::number(azimuth.numel()));
    QStringList dataStream;

    for (int a=0; a < azimuth.numel(); a++)
        dataStream << QString::number(azimuth(a));
    QDomText aText = document.createTextNode(dataStream.join("\n"));
    azimuth_root.appendChild(aText);
    data_support.appendChild(azimuth_root);


    QDomElement zenith_root = document.createElement("zenith_support");

    zenith_root.setAttribute("size",QString::number(zenith.numel()));

    dataStream.clear();
    for (int z=0; z < zenith.numel(); z++)
        dataStream << QString::number(zenith(z));

    QDomText zText = document.createTextNode(dataStream.join("\n"));
    zenith_root.appendChild(zText);
    data_support.appendChild(zenith_root);

    QDomElement freq_root = document.createElement("frequency");
    freq_root.setAttribute("size",QString::number(freqs.numel()));
    // Freqs
    dataStream.clear();
    for (int f=0; f < freqs.numel(); f++)
        dataStream << QString::number(freqs(f));

    QDomText fText = document.createTextNode(dataStream.join("\n"));
    freq_root.appendChild(fText);
    data_support.appendChild(freq_root);



    QDomElement data_et = document.createElement("ETheta");
    dataStream.clear();

    data_et.setAttribute("size",
                         QString::number(azimuth.numel() * zenith.numel() * freqs.numel()));

    for (int a=0; a < azimuth.numel(); a++)
        for (int z = 0; z < zenith.numel(); z++)
            for (int f =0 ; f < freqs.numel(); f++)
            {
                double etr = et(a,z,f).real();
                double eti = et(a,z,f).imag();
                dataStream << QString::number(etr);
                dataStream << QString::number(eti);
            }
    data_et.appendChild(document.createTextNode(dataStream.join("\n")));
    data.appendChild(data_et);


    QDomElement data_ep = document.createElement("EPhi");

    data_ep.setAttribute("size",
                         QString::number(azimuth.numel() * zenith.numel() * freqs.numel()));


    dataStream.clear();

    for (int a=0; a < azimuth.numel(); a++)
        for (int z = 0; z < zenith.numel(); z++)
            for (int f =0 ; f < freqs.numel(); f++)
            {
                double epr = ep(a,z,f).real();
                double epi = ep(a,z,f).imag();
                dataStream << QString::number(epr);
                dataStream << QString::number(epi);
            }
    data_ep.appendChild(document.createTextNode(dataStream.join("\n")));
    data.appendChild(data_ep);
    QDomElement freqs_s11 = document.createElement("S11_freqs");
    dataStream.clear();

    for (int f = 0; f < s11_freqs.numel(); f++)
        dataStream << QString::number(s11_freqs(f));

    freqs_s11.appendChild(document.createTextNode(dataStream.join("\n")));
    data.appendChild(freqs_s11);

    QDomElement data_s11 = document.createElement("S11");
    dataStream.clear();

    for (int f = 0; f < s11_freqs.numel(); f++)
    {
        dataStream << QString::number(s11(f).real());
        dataStream << QString::number(s11(f).imag());
    }
    data_s11.appendChild(document.createTextNode(dataStream.join("\n")));
    data.appendChild(data_s11);
    return true;
}
//-------------------------------------------------------
bool antenna::load_xml()
{

    QDomDocument document;

    if (!antenna_file.open(QIODevice::ReadOnly ))
    {
        return false;
    }
    document.setContent(&antenna_file);
    antenna_file.close();

    return load_xml(document);
}

//-------------------------------------------------------
bool antenna::load_xml(QDomDocument& document)
{
    NDArray newaz;
    NDArray newzen;
    NDArray newgain;
    NDArray newfreqs;
    ComplexNDArray newet;
    ComplexNDArray newep;
    ComplexNDArray news11;
    NDArray news11freqs;
    QDomElement root = document.firstChildElement("ARIA_Antenna");


    QString xml_ver_string = root.attribute("xml_dataformat");

    QVersionNumber xml_ver = QVersionNumber::fromString(xml_ver_string);

    QVersionNumber current_xml({XML_ANTENNAFILE_MAJOR,XML_ANTENNAFILE_MINOR, XML_ANTENNAFILE_SUBVER});

    if (xml_ver > current_xml)
        return false;

    QDomElement data_configuration = root.firstChildElement("data_configuration");


    if (data_configuration.isNull()) return false;


    QDomElement azimuth_root = data_configuration.firstChildElement("azimuth_support");
    if (azimuth_root.isNull()) return false;
    bool bOk;
    int az_size = azimuth_root.attribute("size").toInt(&bOk);
    if (!bOk) return false;
    QStringList dataStream(azimuth_root.text().split("\n"));
    if (dataStream.count()!=az_size) return false;
    newaz.resize(dim_vector(1,az_size));

    for (int a = 0; a < az_size; a++)
    {
        double az = dataStream[a].toDouble(&bOk);
        if (!bOk) return false;
        newaz(a) = az;
    }


    QDomElement zenith_root = data_configuration.firstChildElement("zenith_support");
    if (zenith_root.isNull()) return false;
    int zen_size = zenith_root.attribute("size").toInt(&bOk);
    if (!bOk) return false;
    dataStream.clear();
    dataStream = zenith_root.text().split("\n");

    if (dataStream.count()!=zen_size) return false;
    newzen.resize(dim_vector(1,zen_size));

    for (int z = 0; z < zen_size; z++)
    {
        double zen = dataStream[z].toDouble(&bOk);
        if (!bOk) return false;
        newzen(z) = zen;
    }

    QDomElement freq_root = data_configuration.firstChildElement("frequency");
    if (freq_root.isNull()) return false;
    int freq_size = freq_root.attribute("size").toInt(&bOk);
    if (!bOk) return false;
    dataStream.clear();
    dataStream = freq_root.text().split("\n");

    if (dataStream.count()!=freq_size) return false;
    newfreqs.resize(dim_vector(1,freq_size));

    for (int f = 0; f < freq_size; f++)
    {
        double freq = dataStream[f].toDouble(&bOk);
        if (!bOk) return false;
        newfreqs(f) = freq;
    }

    QDomElement data               = root.firstChildElement("antenna_data");
    if (data.isNull()) return false;
    newet.resize(dim_vector(az_size, zen_size, freq_size));
    newep.resize(dim_vector(az_size, zen_size, freq_size));
    news11.resize(dim_vector(1,freq_size));


    QDomElement theta_root = data.firstChildElement("ETheta");
    if (theta_root.isNull()) return false;

    int theta_size = theta_root.attribute("size").toInt(&bOk);
    if (!bOk) return false;
    if (theta_size!=(az_size*zen_size*freq_size)) return false;

    dataStream.clear();
    dataStream = theta_root.text().split("\n");
    if (dataStream.count()!=2*az_size*zen_size*freq_size) return false;

    int index=0;
    for (int a=0; a < az_size; a++)
        for (int z=0; z < zen_size; z++)
            for (int f=0; f < freq_size; f++)
            {
                double re = dataStream[index++].toDouble(&bOk);
                if (!bOk) return false;
                double im = dataStream[index++].toDouble(&bOk);
                if (!bOk) return false;
                newet(a,z,f) = Complex(re,im);
            }

    QDomElement phi_root = data.firstChildElement("EPhi");
    if (phi_root.isNull()) return false;

    int phi_size = phi_root.attribute("size").toInt(&bOk);
    if (!bOk) return false;
    if (phi_size!=(az_size*zen_size*freq_size)) return false;


    dataStream.clear();
    dataStream = phi_root.text().split("\n");
    if (dataStream.count()!=2*az_size*zen_size*freq_size) return false;



    index=0;
    for (int a=0; a < az_size; a++)
        for (int z=0; z < zen_size; z++)
            for (int f=0; f < freq_size; f++)
            {
                double re = dataStream[index++].toDouble(&bOk);
                if (!bOk) return false;
                double im = dataStream[index++].toDouble(&bOk);
                if (!bOk) return false;
                newep(a,z,f) = Complex(re,im);
            }

    QDomElement s11_freqs_data = data.firstChildElement("S11_freqs");
    if (s11_freqs_data.isNull()) return false;
    dataStream.clear();
    dataStream = s11_freqs_data.text().split("\n");

    freq_size = dataStream.count();

    news11freqs.resize(dim_vector(1, freq_size));
    news11.resize(dim_vector(1, freq_size));

    index=0;
    for (int f=0; f < freq_size; f++)
    {
        news11freqs(f) = dataStream[index++].toDouble(&bOk);
        if (!bOk) return false;
    }


    QDomElement s11_root = data.firstChildElement("S11");
    if (s11_root.isNull()) return false;
    dataStream.clear();
    dataStream = s11_root.text().split("\n");
    if (dataStream.count()!=2*freq_size) return false;

    index=0;
    for (int f=0; f < freq_size; f++)
    {
        double re = dataStream[index++].toDouble(&bOk);
        if (!bOk) return false;
        double im = dataStream[index++].toDouble(&bOk);
        if (!bOk) return false;
        news11(f) = Complex(re,im);
    }

    calcGain();
    freqs   = newfreqs;
    azimuth = newaz;
    zenith  = newzen;
    et      = newet;
    ep      = newep;
    gain    = newgain;
    s11     = news11;
    s11_freqs=news11freqs;

    bModified = false;
    return true;
}

//-------------------------------------------------------
bool antenna::hasFile()
{
    return antenna_file.isOpen();
}

//-------------------------------------------------------
octave_value antenna::toOctave()
{
    octave_value out;
    out.subsref(".", {ovl("azimuth")})      = azimuth;
    out.subsref(".", {ovl("zenith")})       = zenith;
    out.subsref(".", {ovl("frequencies")})  = freqs;
    out.subsref(".", {ovl("et")})           = et;
    out.subsref(".", {ovl("ep")})           = ep;
    calcGain();
    out.subsref(".", {ovl("gain")})         = gain;
    out.subsref(".", {ovl("s11")})          = s11;
    return out;
}
//-------------------------------------------------------
bool         antenna::fromOctave(octave_value& val)
{
    antenna temp;
    if (!val.isstruct()) return false;
    // names
    octave_value field  = val.subsref(".", {ovl("antenna_name")});
    if ((!(field.is_string()))||(field.isempty()))
        return false;

    temp.set_name(QString::fromStdString(field.string_value()));

    // azimuth
    field  = val.subsref(".", {ovl("azimuth")});
    if ((!(field.is_real_matrix()))||(field.isempty()))
        return false;

    temp.azimuth = field.array_value();

    // zenith
    field  = val.subsref(".", {ovl("zenith")});
    if ((!(field.is_real_matrix()))||(field.isempty()))
        return false;

    temp.zenith = field.array_value();

    // frequencies
    field  = val.subsref(".", {ovl("frequencies")});
    if ((!(field.is_real_matrix()))||(field.isempty()))
        return false;

    temp.freqs = field.array_value();

    // et
    field  = val.subsref(".", {ovl("et")});
    if ((!(field.is_complex_matrix()))||(field.isempty()))
        return false;

    temp.et = field.complex_array_value();

    // ep
    field  = val.subsref(".", {ovl("ep")});
    if ((!(field.is_complex_matrix()))||(field.isempty()))
        return false;

    temp.ep = field.complex_array_value();

    // s11
    field  = val.subsref(".", {ovl("s11")});
    if ((!(field.is_real_matrix()))||(field.isempty()))
        return false;

    temp.s11 = field.complex_array_value();

    *this = temp;

    return true;
}
//-----------------------------------------------------------
void antenna::reset_fields()
{
    ep.fill(Complex(0,0));
    et.fill(Complex(0,0));
    gain.fill(0);
    s11.fill(Complex(0,0));
}

//-----------------------------------------------------------
octave_value antenna::get_common_support(NDArray arr1, NDArray arr2)
{
    double dx2  = 9e99;
    double min2 = 9e99;
    double max2  = -9e99;
    for (int n = 0; n < arr2.numel(); n++)
    {
        double dx;
        double x=0;
        if (n>0)
        {
            dx = arr2(n) - x;
            if (dx < dx2) dx2 = dx;
        }
        x = arr2(n);
        if (x < min2) min2 = x;
        if (x > max2) max2 = x;
    }

    double dx1  = 9e99;
    double min1 = 9e99;
    double max1  = -9e99;
    for (int n = 0; n < arr1.numel(); n++)
    {
        double dx;
        double x=0;
        if (n>0)
        {
            dx = arr1(n) - x;
            if (dx < dx1) dx1 = dx;
        }
        x = arr1(n);
        if (x < min1) min1 = x;
        if (x > max1) max1 = x;
    }

    double start = min2 > min1 ? min2 : min1;
    double stop  = max2 < max1 ? max2 : max2;
    double dx    = dx2 - dx1   ? dx2  : dx1;
    int    nstep = ceil(stop-start-dx)/dx;

    return octave_value (linspace(start,stop,nstep));
}
//-----------------------------------------------------------
void antenna::resample_antennas(antenna& ant2, bool resample_ant2)
{

    // Avoid extrapolation (i.e. take max freq min and min freq max)
    // 1. Go with frequencies:
    octave_value new_freq_support       = get_common_support(freqs,ant2.freqs);
    octave_value new_azimuth_support    = get_common_support(azimuth, ant2.azimuth);
    octave_value new_zenith_support     = get_common_support(zenith, ant2.zenith);

    octave_value_list in(7);
    in(0)=azimuth;
    in(1)=zenith;
    in(2)=freqs;
    in(3)=et;
    in(4)=new_azimuth_support;
    in(5)=new_zenith_support;
    in(6)=new_freq_support;

    // Resample *this
    et = octave::feval("interp3",in,1)(0).complex_array_value();

    in(3)=ep;
    ep = octave::feval("interp3",in,1)(0).complex_array_value();

    azimuth = new_azimuth_support.array_value();
    zenith  = new_zenith_support.array_value();
    freqs   = new_freq_support.array_value();


    // Resample ant2
    if (!resample_ant2) return;

    in(0) = ant2.azimuth;
    in(1) = ant2.zenith;
    in(2) = ant2.freqs;

    in(3) = ant2.et;

    // Resample *this
    ant2.et = octave::feval("interp3",in,1)(0).complex_array_value();

    in(3) = ant2.ep;
    ant2.ep = octave::feval("interp3",in,1)(0).complex_array_value();

    ant2.azimuth = new_azimuth_support.array_value();
    ant2.zenith  = new_zenith_support.array_value();
    ant2.freqs   = new_freq_support.array_value();



}


//-----------------------------------------------------------
void antenna::resample_antennas(antenna_model_vector& antennas)
{
    if (antennas.count()<2) return;

    octave_value new_freq_support       = get_common_support(antennas[0]->freqs     ,   antennas[1]->freqs);
    octave_value new_azimuth_support    = get_common_support(antennas[0]->azimuth   ,   antennas[1]->azimuth);
    octave_value new_zenith_support     = get_common_support(antennas[0]->zenith    ,   antennas[1]->zenith);

    if (antennas.count()>2)
    {
        for (int n=2; n < antennas.count(); n++)
        {
            new_freq_support       = get_common_support(antennas[n]->freqs     ,   new_freq_support.array_value());
            new_azimuth_support    = get_common_support(antennas[n]->azimuth   ,   new_azimuth_support.array_value());
            new_zenith_support     = get_common_support(antennas[n]->zenith    ,   new_zenith_support.array_value());
        }
    }

    octave_value_list in(7);
    in(4)=new_azimuth_support;
    in(5)=new_zenith_support;
    in(6)=new_freq_support;

    for (int n=0 ; n < antennas.count(); n++)
    {
        in(0)=antennas[n]->azimuth;
        in(1)=antennas[n]->zenith;
        in(2)=antennas[n]->freqs;
        in(3)=antennas[n]->et;
        antennas[n]->et = octave::feval("interp3",in,1)(0).complex_array_value();
        in(3)=antennas[n]->ep;
        antennas[n]->ep = octave::feval("interp3",in,1)(0).complex_array_value();
        antennas[n]->azimuth = new_azimuth_support.array_value();
        antennas[n]->zenith  = new_zenith_support.array_value();
        antennas[n]->freqs   = new_freq_support.array_value();
        antennas[n]->calcGain();
    }

}
//------------------------------------------------------------------
std::shared_ptr<antenna> antenna::clone()
{
    return  std::make_shared<antenna>(*this);
}

//------------------------------------------------------------------
bool    antenna::operator == (const antenna& ant)
{
    if (!is_equal(azimuth, ant.azimuth)) return false;
    if (!is_equal(zenith, ant.zenith)) return false;
    if (!is_equal(freqs, ant.freqs)) return false;
    if (!is_equal(et, ant.et)) return false;
    if (!is_equal(ep, ant.ep)) return false;
    if (!is_equal(s11, ant.s11)) return false;
    if (!is_equal(s11_freqs, ant.s11_freqs)) return false;
    return true;
}
