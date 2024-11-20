/*
 *  ARIA Sensing 2024
 *  Author: Cacciatori Alessio
 *  This program is licensed under LGPLv3.0
 */

#ifndef ANTENNAINSTANCE_H
#define ANTENNAINSTANCE_H

#include <QString>
#include <octave.h>
#include <ov.h>
#include <ovl.h>
#include <antenna.h>
#include <radarproject.h>
#include <QtXml>

class antennaInstance : public antenna
{
private:
    antenna_model_pointer model;
    NDArray rotation;
    NDArray position;
    double delay;
    double loss;
    int                 _freq_of_interest;
    QString             _ant_name;
    ComplexNDArray      _d_p, _d_t;
    NDArray             _gain_eq;
//--------------------------------------------------
// Actual values
//--------------------------------------------------
    void        update_fields_from_model();
    void        copy_base_data(antennaInstance& ant2);
    void        copy_base_data(const antennaInstance& ant2);

public:
    antennaInstance(antenna_model_pointer antenna_model = nullptr);
    antennaInstance(antennaInstance &ant);
    antennaInstance(const antennaInstance &ant);
    ~antennaInstance();


    octave_value        toOctave();
    bool                fromOctave(octave_value& val);
    octave_value        calculateResponse(octave_value azimuth_range, octave_value zenith_range, octave_value time, octave_value input_wfm);

    antennaInstance&    recalcFreqs(octave_value freqSupport);
    antennaInstance&    recalcAzimuthZenith(octave_value azimuthSupport, octave_value zenithSupport);

    bool                setTranslationRotationDelayLoss(octave_value position, octave_value rotation, double delay_ns, double loss_db);
    antennaInstance     operator +(antennaInstance& ant2); // Combine two antennas in addictive way (e.g. tx1+tx2)
    antennaInstance     operator ()(double delay); // Apply a time delay to "this" and return the resulting field
    antennaInstance     operator [](double phase); // Apply a phase delay to "this" and return the resulting field
    antennaInstance     operator *(antennaInstance& ant2); // Multuply two antennas (e.g. tx*rx)

    void                apply_phase(double phase);
    void                apply_delay(double delay);
    void                add(const antennaInstance& ant2);
    void                mult(const antennaInstance& ant2);
    void                copy_from(const antennaInstance& ant2);
    void                copy_from(antennaInstance& ant2);
    antennaInstance      operator = (antennaInstance& ant2);
    antennaInstance      operator = (const antennaInstance& ant2);

    bool                 has_same_support(antennaInstance &ant2);
    void                 resize_as(antennaInstance& ant2);
    void                 resize_as(const antennaInstance& ant2);
    void                 calculate_plane_wave();

    octave_value          get_phase_center() {return octave_value(position);}
    antenna_model_pointer get_antenna_model() {return model;}
    void                  set_antenna_model(antenna_model_pointer antenna_model);

    std::shared_ptr<antennaInstance> clone();

    double get_delay() {return delay;}
    void   set_delay(double delay_s) {delay = delay_s;}
    double get_loss() {return loss;}
    void   set_loss(double loss_db) {loss = loss_db;}
    void   set_rotation(bool bPhi_nbTheta, double rotation_deg) {rotation(bPhi_nbTheta ? 0:1) = rotation_deg*M_PI/180;}
    double get_rotation(bool bPhi_nbTheta) {return rotation(bPhi_nbTheta ? 0:1)*180/M_PI;}
    void   set_position(int coord, double pos) {position(coord)=pos;}
    double get_position(int coord)  {return position(coord);}

    bool                    save_xml(QDomDocument& owner, QDomElement& root);
    static antenna_pointer  load_xml(QDomDocument& owner, QDomElement& root, QString basedir="", antenna_model_pointer ref_model=nullptr);

    void                    set_antenna_name(QString antenna_name) {_ant_name = antenna_name;}
    QString                 get_antenna_name(void) const           {return _ant_name;}

    bool                    calculate_over_freq_azimuth_zenith_support(NDArray freq_support, NDArray azimuth_support, NDArray zenith_support);
    void                    calculate_directivity_aeff();
    void                    calc_gain_equivalent();
    NDArray                 gain_equivalent();
    void                    set_freq_of_interest(int nf=-1) {_freq_of_interest = nf;}
    int                     freq_of_interest()              {return _freq_of_interest;}

    bool                    operator == (const antennaInstance& ant);

};

#endif // ANTENNAINSTANCE_H
