/*
 *  ARIA Sensing 2024
 *  Author: Cacciatori Alessio
 *  This program is licensed under LGPLv3.0
 */
#ifndef ARIAUTILS_H
#define ARIAUTILS_H
#include <complex>
#include <QtSql/QSqlDatabase>
#include <QtSql/QSqlQuery>
#include <QFile>
#include <QDataStream>
#include <QTextStream>
#include <iostream>
#include <ov.h>

#include <float.h>

bool expectChar(QTextStream& in, char cexp);
bool readVal(QTextStream& in, QString& out, char endChar);
bool readVal(QTextStream& in, quint64& out, char endChar);
bool readVal(QTextStream& in, double& out, char endChar);
bool readVal(QTextStream& in, Complex& out, char endChar);
bool checkString(QTextStream& in, const QString& strCheck);
QTextStream& operator << (QTextStream &out, const Complex &value);
QTextStream& operator >> (QTextStream &in, Complex &value);

#define COORD_X 0
#define COORD_Y 1
#define COORD_Z 2

#define COORD_R 0
#define COORD_AZ 1
#define COORD_ZEN 2

#define COORD_PHI 1
#define COORD_THETA 2

#define MU0     1.25663706e-06
#define EPS0    8.8541878128e-12
#define _C0      299792458
#define ETA0    376.73031
#define ONE_OVERETA0 0.0026544187


NDArray     polar_to_cartesian(double r, double az, double zen);
NDArray     polar_to_cartesian(NDArray polar_vector);
NDArray     polar_to_cartesian(NDArray polar_vector, NDArray center_cartesian);
NDArray     polar_to_cartesian(double r, double az, double zen, double xc, double yc, double zc);
NDArray     cartesian_to_polar(double x, double y, double z);
NDArray     cartesian_to_polar(NDArray cartesian_vector);
NDArray     cartesian_to_polar(NDArray cartesian_vector, NDArray center_cartesian);
NDArray     cartesian_to_polar(double x, double y, double z, double xc, double yc, double zc);
void        cartesian_to_polar(double x, double y, double z, double* r, double* az, double* zen);
Matrix      get_rotation_matrix(double rot_x, double rot_y, double rot_z);
NDArray     convert_theta_versor_to_cartesian(double theta, double phi);
NDArray     convert_phi_versor_to_cartesian(double theta, double phi);
NDArray     convert_theta_versor_to_cartesian(double theta, double phi, NDArray center);
NDArray     convert_phi_versor_to_cartesian(double theta, double phi, NDArray center);

NDArray     remap_theta_phi_to_offset(NDArray vect_cartesian, NDArray position, NDArray offset);
NDArray     remap_theta_phi_to_offset_polar(NDArray vect_polar, NDArray position, NDArray offset);
NDArray     cross_product(NDArray v1, NDArray v2);
double      dot(NDArray v1, NDArray v2);
void        normalize(NDArray& v);

ComplexNDArray  interp1( NDArray &x, ComplexNDArray &y, NDArray &x_new );
NDArray         interp1( NDArray &x, NDArray &y, NDArray &x_new );

bool     is_equal(const NDArray& x, const NDArray& y);
bool     is_equal(const ComplexNDArray& x, const ComplexNDArray& y);
bool     is_equal(const int8NDArray& x, const int8NDArray& y);
bool     is_equal(const uint8NDArray& x, const uint8NDArray& y);
bool     is_equal(const int16NDArray& x, const int16NDArray& y);
bool     is_equal(const uint16NDArray& x, const uint16NDArray& y);
bool     is_equal(const int32NDArray& x, const int32NDArray& y);
bool     is_equal(const uint32NDArray& x, const uint32NDArray& y);
bool     is_equal(const FloatNDArray& x, const FloatNDArray& y);
bool     is_equal(const charNDArray& x, const charNDArray& y);

template<typename T> bool	 equal_t(const Array<T> &x, const Array<T>& y);

const double EPS_MIN_DOUBLE= 128 * DBL_EPSILON;
const float  EPS_MIN_FLOAT = 128 * FLT_EPSILON;

bool nearly_equal(double a, double b, double  epsilon = EPS_MIN_DOUBLE, double abs_th = DBL_MIN);
bool nearly_equal(float  a, float  b, float   epsilon = EPS_MIN_FLOAT, float abs_th = FLT_MIN);
bool            find_azimuth_zenith(NDArray& azimuth, NDArray& zenith, double az, double zen,
                                    int& a0, int& a1,
                                    int& z0, int& z1,
                                    double& da, double& dz);

#define SQR(x) ((x)*(x))
#endif // ARIAUTILS_H
