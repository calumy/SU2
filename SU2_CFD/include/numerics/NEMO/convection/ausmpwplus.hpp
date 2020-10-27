/*!
 * \file ausmpwplus.hpp
 * \brief Declaration of numerics classes for the AUSM family of schemes in NEMO - AUSMPWPLUS.
 * \author F. Palacios, W.Maier, C. Garbacz
 * \version 7.0.6 "Blackbird"
 *
 * SU2 Project Website: https://su2code.github.io
 *
 * The SU2 Project is maintained by the SU2 Foundation
 * (http://su2foundation.org)
 *
 * Copyright 2012-2020, SU2 Contributors (cf. AUTHORS.md)
 *
 * SU2 is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * SU2 is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with SU2. If not, see <http://www.gnu.org/licenses/>.
 */

#pragma once

#include "../CNEMONumerics.hpp"

/*!
 * \class CUpwAUSM_NEMO
 * \brief Class for solving an approximate Riemann AUSM.
 * \ingroup ConvDiscr
 * \author F. Palacios, W.Maier, C. Garbacz
 * \version 7.0.6 "Falcon"
 */
class CUpwAUSMPWplus_NEMO : public CNEMONumerics {
private:
  su2double *FcL, *FcR;
  su2double *dmLdL, *dmLdR, *dmRdL, *dmRdR;
  su2double *dmLPdL, *dmLPdR, *dmRMdL, *dmRMdR;
  su2double *dmbLPdL, *dmbLPdR, *dmbRMdL, *dmbRMdR;
  su2double *dpLPdL, *dpLPdR, *dpRMdL, *dpRMdR;
  su2double *dHnL, *dHnR;
  su2double *daL, *daR;
  su2double *dPdU_i, *dPdU_j;
 
public:

  /*!
   * \brief Constructor of the class.
   * \param[in] val_nDim - Number of dimensions of the problem.
   * \param[in] val_nVar - Number of variables of the problem.
   * \param[in] config - Definition of the particular problem.
   */
  CUpwAUSMPWplus_NEMO(unsigned short val_nDim, unsigned short val_nVar, unsigned short val_nPrimVar, unsigned short val_nPrimVarGrad, CConfig *config);

  /*!
   * \brief Destructor of the class.
   */
  ~CUpwAUSMPWplus_NEMO(void);

  /*!
   * \brief Compute the Roe's flux between two nodes i and j.
   * \param[in] config - Definition of the particular problem.
   */
  ResidualType<> ComputeResidual(const CConfig* config) final;
};
