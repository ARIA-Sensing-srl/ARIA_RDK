/*
 *  ARIA Sensing 2024
 *  Author: Cacciatori Alessio
 *  This program is licensed under LGPLv3.0
 */

#ifndef ANTENNA_H
#define ANTENNA_H

#include <QString>

#include <octave.h>
#include <ov.h>
#include <math.h>
#include <radarproject.h>
#include <memory>
#include <QtXml>


#undef DEBUG_FOI
#undef DEBUG_FOI_FULL


NDArray dB_field(NDArray input, bool db20=true);
NDArray dB_field(ComplexNDArray input);

typedef class antennaInstance*                                      antenna_pointer;
typedef QVector<antenna_pointer>                                    antenna_array;
typedef class antenna*                                              antenna_model_pointer;
typedef QVector<antenna_model_pointer>                              antenna_model_vector;


#include <QFile>
class antenna : public projectItem
{
private:
    NDArray        azimuth;     // Support for azimuth angles
    NDArray        zenith;      // Support for zenith angles
    NDArray        freqs;       // Frequency support
    ComplexNDArray et;          // farfield Etheta (calculated at 1m distance). It is be in the form et(azim,zen,freq)
    ComplexNDArray ep;          // farfield Ephi   (calculated at 1m distance). It is be in the form ep(azim,zen,freq)
    NDArray        gain;        // Resulting gain
    ComplexNDArray s11;         // Complex S11 value (referred to 50Ohms!)
    NDArray        s11_freqs;
    bool           gainCalculated;

    void           calcGain();
    bool           bModified;
    QFile          antenna_file;
public:
    antenna(projectItem* parent = nullptr);
    antenna(QString filename,projectItem* parent = nullptr);
    antenna(antenna& ant2);
    antenna(const antenna& ant2);
    void set_filename(QString filename);
    ~antenna();
//------------------------------------------
    bool         loadFarField(const QStringList &filelist);
//------------------------------------------
    octave_value toOctave();
    bool         fromOctave(octave_value& val);
//------------------------------------------
    octave_value    getGain(octave_value azimuths, octave_value zeniths, octave_value freqs);
    double          getGainInterp(double azimuth, double zen, double f);
    bool            getEtEpInterp(double az, double zen, double f, Complex& etval, Complex& epval);
    ComplexNDArray  getEp(int freqid);
    ComplexNDArray  getEt(int freqid);

//------------------------------------------
    antenna&     operator = (antenna& ant2);
    antenna&     operator = (const antenna& ant2);

//------------------------------------------
    void         setFields(const octave_value& azsupport,
                           const octave_value& zensupport,
                           const octave_value& freqsupport,
                           const octave_value& EtField,
                           const octave_value& EpField);
    double       calculateRadiatedPwrAtFreqInterp(double freq); // Return radiated power @ freq in W/m^2
    double       calculateRadiatedPwrAtFreq(int freqid);
    double       calculatePowerFlowInterp(double a, double z, double f);
    double       calculatePowerFlow(int aid, int zid, int fid);
    void         insertEpEtFreq(double freq, ComplexNDArray Ep, ComplexNDArray Et);
    bool         insertFreqs(const QVector<double>& newfreq, ComplexNDArray Et, ComplexNDArray Ep);
    double       dS(double azimuth_angle, double zenith_angle,  double r=1.0);
    double       getFreqMin();
    double       getFreqMax();
    NDArray      getFreqs();
    NDArray      getAzimuth();
    NDArray      getZenith();
    NDArray      getGain(int freqid);
    bool         isModified() {return bModified;}
//------------------------------------------
    bool         save(QString filename="");
    bool         load(QString filename="");
    bool         save_xml();
    bool         save_xml(QDomDocument& document);
    bool         load_xml();
    bool         load_xml(QDomDocument& document);

    bool         hasFile();
//------------------------------------------
    void         reset_fields();
    void         resample_antennas(antenna& ant2, bool resample_ant2=false);
//------------------------------------------
    NDArray             get_s11_freqs() {return s11_freqs;}
    ComplexNDArray      get_s11()       {return s11;}
    void                set_s11(NDArray fsupport, ComplexNDArray S11) {s11_freqs = fsupport; s11 = S11;}
    friend class antennaInstance;

    static octave_value get_common_support(NDArray arr1, NDArray arr2);
    static void         resample_antennas(antenna_model_vector& antennas);
    std::shared_ptr<antenna> clone();

    QString             get_name() const {return  projectItem::get_name();}
    bool                operator == (const antenna& ant);
    const ComplexNDArray& getEp() {return ep;}
    const ComplexNDArray& getEt() {return et;}
};

#endif // ANTENNA_H
