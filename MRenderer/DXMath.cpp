#include "DXMath.h"

XMMATRIX operator*(const XMMATRIX& m1, const XMMATRIX& m2)
{
	return XMMatrixMultiply(m1, m2);
}
