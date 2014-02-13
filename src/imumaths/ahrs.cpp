#include "ahrs.h"

imu::Quaternion q;
imu::Quaternion offset;
imu::Quaternion body;

volatile float beta = 0.1;
unsigned long last_millis = 0;

void MadgwickAHRSupdate(imu::Vector<3> g, imu::Vector<3> a, imu::Vector<3> m, float dt);


void uimu_ahrs_init(uint32_t ts, imu::Vector<3> acc, imu::Vector<3> mag)
{
    imu::Vector<3> down = acc.invert();
    imu::Vector<3> east = down.cross(mag);
    imu::Vector<3> north = east.cross(down);

    down.normalize();
    east.normalize();
    north.normalize();

    imu::Matrix<3> m;
    m.vector_to_row(north, 0);
    m.vector_to_row(east, 1);
    m.vector_to_row(down, 2);

    q.fromMatrix(m);
    last_millis = ts;
}


void  uimu_ahrs_set_offset(imu::Quaternion o)
{
	offset = o;
}

void uimu_ahrs_set_beta(float b)
{
    beta = b;
}

void uimu_ahrs_iterate(uint32_t ts, imu::Vector<3> acc, imu::Vector<3> ang_vel, imu::Vector<3> mag)
{
    float dt = ts - last_millis;
    last_millis = ts;

    dt /= 1000.0;

    MadgwickAHRSupdate(ang_vel, acc, mag, dt);

    q.normalize();

	body = q * offset.conjugate();
}


imu::Vector<3> uimu_ahrs_get_euler()
{
    imu::Vector<3> euler = body.toEuler();
    euler.toDegrees();
    return euler;
}

imu::Matrix<3> uimu_ahrs_get_matrix()
{
    return body.toMatrix();
}

imu::Quaternion uimu_ahrs_get_quaternion()
{
    return body;
}

imu::Quaternion uimu_ahrs_get_imu_quaternion()
{
	return q;
}


void MadgwickAHRSupdate(imu::Vector<3> g, imu::Vector<3> a, imu::Vector<3> m, float dt)
{
    imu::Vector<4> s;
    imu::Vector<4> qDot;
    float hx, hy;
    float _2q0mx, _2q0my, _2q0mz, _2q1mx, _2bx, _2bz, _4bx, _4bz, _2q0, _2q1, _2q2, _2q3, _2q0q2, _2q2q3, q0q0, q0q1, q0q2, q0q3, q1q1, q1q2, q1q3, q2q2, q2q3, q3q3;

    if(isnan(m.magnitude()) || isinf(m.magnitude())) 
        return;

    qDot[0] = 0.5f * (-q.y() * g.x() - q.y() * g.y() - q.z() * g.z());
    qDot[1] = 0.5f * (q.w() * g.x() + q.y() * g.z() - q.z() * g.y());
    qDot[2] = 0.5f * (q.w() * g.y() - q.x() * g.z() + q.z() * g.x());
    qDot[3] = 0.5f * (q.w() * g.z() + q.x() * g.y() - q.y() * g.x());

    if(!(isnan(a.magnitude()))) 
	{
        a.normalize();
        m.normalize();

        // Auxiliary variables to avoid repeated arithmetic
        _2q0mx = 2.0f * q.w() * m.x();
        _2q0my = 2.0f * q.w() * m.y();
        _2q0mz = 2.0f * q.w() * m.z();
        _2q1mx = 2.0f * q.x() * m.x();
        _2q0 = 2.0f * q.w();
        _2q1 = 2.0f * q.x();
        _2q2 = 2.0f * q.y();
        _2q3 = 2.0f * q.z();
        _2q0q2 = 2.0f * q.w() * q.y();
        _2q2q3 = 2.0f * q.y() * q.z();
        q0q0 = q.w() * q.w();
        q0q1 = q.w() * q.x();
        q0q2 = q.w() * q.y();
        q0q3 = q.w() * q.z();
        q1q1 = q.x() * q.x();
        q1q2 = q.x() * q.y();
        q1q3 = q.x() * q.z();
        q2q2 = q.y() * q.y();
        q2q3 = q.y() * q.z();
        q3q3 = q.z() * q.z();

        // Reference direction of Earth's magnetic field
        hx = m.x() * q0q0 - _2q0my * q.z() + _2q0mz * q.y() + m.x() * q1q1 + _2q1 * m.y() * q.y() + _2q1 * m.z() * q.z() - m.x() * q2q2 - m.x() * q3q3;
        hy = _2q0mx * q.z() + m.y() * q0q0 - _2q0mz * q.x() + _2q1mx * q.y() - m.y() * q1q1 + m.y() * q2q2 + _2q2 * m.z() * q.z() - m.y() * q3q3;
        _2bx = sqrt(hx * hx + hy * hy);
        _2bz = -_2q0mx * q.y() + _2q0my * q.x() + m.z() * q0q0 + _2q1mx * q.z() - m.z() * q1q1 + _2q2 * m.y() * q.z() - m.z() * q2q2 + m.z() * q3q3;
        _4bx = 2.0f * _2bx;
        _4bz = 2.0f * _2bz;

        // Gradient decent algorithm corrective step
        s[0] = -_2q2 * (2.0f * q1q3 - _2q0q2 - a.x()) + _2q1 * (2.0f * q0q1 + _2q2q3 - a.y()) - _2bz * q.y() * (_2bx * (0.5f - q2q2 - q3q3) + _2bz * (q1q3 - q0q2) - m.x()) + (-_2bx * q.z() + _2bz * q.x()) * (_2bx * (q1q2 - q0q3) + _2bz * (q0q1 + q2q3) - m.y()) + _2bx * q.y() * (_2bx * (q0q2 + q1q3) + _2bz * (0.5f - q1q1 - q2q2) - m.z());

        s[1] = _2q3 * (2.0f * q1q3 - _2q0q2 - a.x()) + _2q0 * (2.0f * q0q1 + _2q2q3 - a.y()) - 4.0f * q.x() * (1 - 2.0f * q1q1 - 2.0f * q2q2 - a.z()) + _2bz * q.z() * (_2bx * (0.5f - q2q2 - q3q3) + _2bz * (q1q3 - q0q2) - m.x()) + (_2bx * q.y() + _2bz * q.w()) * (_2bx * (q1q2 - q0q3) + _2bz * (q0q1 + q2q3) - m.y()) + (_2bx * q.z() - _4bz * q.x()) * (_2bx * (q0q2 + q1q3) + _2bz * (0.5f - q1q1 - q2q2) - m.z());

        s[2] = -_2q0 * (2.0f * q1q3 - _2q0q2 - a.x()) + _2q3 * (2.0f * q0q1 + _2q2q3 - a.y()) - 4.0f * q.y() * (1 - 2.0f * q1q1 - 2.0f * q2q2 - a.z()) + (-_4bx * q.y() - _2bz * q.w()) * (_2bx * (0.5f - q2q2 - q3q3) + _2bz * (q1q3 - q0q2) - m.x()) + (_2bx * q.x() + _2bz * q.z()) * (_2bx * (q1q2 - q0q3) + _2bz * (q0q1 + q2q3) - m.y()) + (_2bx * q.w() - _4bz * q.y()) * (_2bx * (q0q2 + q1q3) + _2bz * (0.5f - q1q1 - q2q2) - m.z());

        s[3] = _2q1 * (2.0f * q1q3 - _2q0q2 - a.x()) + _2q2 * (2.0f * q0q1 + _2q2q3 - a.y()) + (-_4bx * q.z() + _2bz * q.x()) * (_2bx * (0.5f - q2q2 - q3q3) + _2bz * (q1q3 - q0q2) - m.x()) + (-_2bx * q.w() + _2bz * q.y()) * (_2bx * (q1q2 - q0q3) + _2bz * (q0q1 + q2q3) - m.y()) + _2bx * q.x() * (_2bx * (q0q2 + q1q3) + _2bz * (0.5f - q1q1 - q2q2) - m.z());

        s.normalize();

        // Apply feedback step
        qDot[0] -= beta * s[0];
        qDot[1] -= beta * s[1];
        qDot[2] -= beta * s[2];
        qDot[3] -= beta * s[3];
    }

    // Integrate rate of change of quaternion to yield quaternion
    q.w() += qDot[0] * dt;
    q.x() += qDot[1] * dt;
    q.y() += qDot[2] * dt;
    q.z() += qDot[3] * dt;
}


