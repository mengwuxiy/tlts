#include "stdafx.h"
#include "SolveData.h"

bool W_D::operator >= (W_D p)
{
	if (this->Km < p.Km)
	{
		return false;
	}
	else if (this->Km > p.Km)
	{
		return true;
	}
	
	if (this->m < p.m)
	{
		return false;
	}
	else if (this->m > p.m)
	{
		return true;
	}

	if (this->mm < p.mm)
	{
		return false;
	}
	return true;
}

bool W_D::operator <= (W_D p)
{
	if (this->Km < p.Km)
	{
		return true;
	}
	else if (this->Km > p.Km)
	{
		return false;
	}

	if (this->m < p.m)
	{
		return true;
	}
	else if (this->m > p.m)
	{
		return false;
	}

	if (this->mm < p.mm)
	{
		return true;
	}
	return false;
}

W_D W_D::operator+(W_D p)
{
	W_D wd = *this; 
	wd.Km += p.Km;
	wd.m += p.m;
	wd.mm += p.mm;

	if (wd.mm > 1000)
	{
		wd.m += 1;
		wd.mm -= 1000;
	}

	if (wd.m >= 1000)
	{
		wd.Km += 1;
		wd.m -= 1000;
	}
	return wd;
}

W_D W_D::operator-(W_D p)
{
	W_D wd = *this;
	wd.Km -= p.Km;
	wd.m -= p.m;
	wd.mm -= p.mm;

	if (wd.mm < 0)
	{
		wd.m -= 1;
		wd.mm += 1000;
	}

	if (wd.m < 0)
	{
		wd.Km -= 1;
		wd.m += 1000;
	}
	return wd;
}
