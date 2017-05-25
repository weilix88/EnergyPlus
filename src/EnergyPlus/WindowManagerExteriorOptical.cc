// EnergyPlus, Copyright (c) 1996-2016, The Board of Trustees of the University of Illinois and
// The Regents of the University of California, through Lawrence Berkeley National Laboratory
// (subject to receipt of any required approvals from the U.S. Dept. of Energy). All rights
// reserved.
//
// If you have questions about your rights to use or distribute this software, please contact
// Berkeley Lab's Innovation & Partnerships Office at IPO@lbl.gov.
//
// NOTICE: This Software was developed under funding from the U.S. Department of Energy and the
// U.S. Government consequently retains certain rights. As such, the U.S. Government has been
// granted for itself and others acting on its behalf a paid-up, nonexclusive, irrevocable,
// worldwide license in the Software to reproduce, distribute copies to the public, prepare
// derivative works, and perform publicly and display publicly, and to permit others to do so.
//
// Redistribution and use in source and binary forms, with or without modification, are permitted
// provided that the following conditions are met:
//
// (1) Redistributions of source code must retain the above copyright notice, this list of
//     conditions and the following disclaimer.
//
// (2) Redistributions in binary form must reproduce the above copyright notice, this list of
//     conditions and the following disclaimer in the documentation and/or other materials
//     provided with the distribution.
//
// (3) Neither the name of the University of California, Lawrence Berkeley National Laboratory,
//     the University of Illinois, U.S. Dept. of Energy nor the names of its contributors may be
//     used to endorse or promote products derived from this software without specific prior
//     written permission.
//
// (4) Use of EnergyPlus(TM) Name. If Licensee (i) distributes the software in stand-alone form
//     without changes from the version obtained under this License, or (ii) Licensee makes a
//     reference solely to the software portion of its product, Licensee must refer to the
//     software as "EnergyPlus version X" software, where "X" is the version number Licensee
//     obtained under this License and may not use a different name for the software. Except as
//     specifically required in this Section (4), Licensee shall not use in a company name, a
//     product name, in advertising, publicity, or other promotional activities any name, trade
//     name, trademark, logo, or other designation of "EnergyPlus", "E+", "e+" or confusingly
//     similar designation, without Lawrence Berkeley National Laboratory's prior written consent.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR
// IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY
// AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR
// CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
// CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
// SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
// THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
// OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
// POSSIBILITY OF SUCH DAMAGE.
//
// You are under no obligation whatsoever to provide any bug fixes, patches, or upgrades to the
// features, functionality or performance of the source code ("Enhancements") to anyone; however,
// if you choose to make your Enhancements available either publicly, or directly to Lawrence
// Berkeley National Laboratory, without imposing a separate written license agreement for such
// Enhancements, then you hereby grant the following license: a non-exclusive, royalty-free
// perpetual license to install, use, modify, prepare derivative works, incorporate into other
// computer software, distribute, and sublicense such enhancements or derivative works thereof,
// in binary and source code form.

#include <assert.h>
#include <algorithm>



// EnergyPlus headers
#include <DataEnvironment.hh>
#include <DataSurfaces.hh>
#include <DataHeatBalance.hh>
#include <DataHeatBalFanSys.hh>
#include <DataGlobals.hh>
#include <InputProcessor.hh>
#include <General.hh>
#include <WindowManager.hh>

// Windows library headers
#include <WCEMultiLayerOptics.hpp>

#include "WindowManagerExteriorOptical.hh"
#include "WindowManagerExteriorData.hh"

namespace EnergyPlus {

  using namespace std;
  using namespace FenestrationCommon;
  using namespace SpectralAveraging;
  using namespace SingleLayerOptics;

  using namespace DataEnvironment;
  using namespace DataSurfaces;
  using namespace DataHeatBalance;
  using namespace DataHeatBalFanSys;
  using namespace DataGlobals;
  using namespace General;

  namespace WindowManager {

    shared_ptr< CBSDFLayer > getBSDFLayer( 
      const shared_ptr< MaterialProperties >& t_Material, 
      const WavelengthRange t_Range ) {
      // SUBROUTINE INFORMATION:
      //       AUTHOR         Simon Vidanovic
      //       DATE WRITTEN   September 2016
      //       MODIFIED       na
      //       RE-ENGINEERED  na

      // PURPOSE OF THIS SUBROUTINE:
      // BSDF will be created in different ways that is based on material type

      shared_ptr< CWCELayerFactory > aFactory = nullptr;
      if( t_Material->Group == WindowGlass ) {
        aFactory = make_shared< CWCESpecularLayerFactory >( t_Material, t_Range );
      } else if( t_Material->Group == WindowBlind ) {
        aFactory = make_shared< CWCEVenetianBlindLayerFactory >( t_Material, t_Range );
      } else if( t_Material->Group == Screen ) {
        aFactory = make_shared< CWCEScreenLayerFactory >( t_Material, t_Range );
      } else if( t_Material->Group == Shade ) {
        aFactory = make_shared< CWCEDiffuseShadeLayerFactory >( t_Material, t_Range );
      }
      return aFactory->getBSDFLayer();
    }

    shared_ptr< CScatteringLayer > getScatteringLayer(
      const shared_ptr< MaterialProperties >& t_Material,
      const WavelengthRange t_Range ) {
      // SUBROUTINE INFORMATION:
      //       AUTHOR         Simon Vidanovic
      //       DATE WRITTEN   May 2017
      //       MODIFIED       na
      //       RE-ENGINEERED  na

      // PURPOSE OF THIS SUBROUTINE:
      // Scattering will be created in different ways that is based on material type

      shared_ptr< CWCELayerFactory > aFactory = nullptr;
      if( t_Material->Group == WindowGlass ) {
        aFactory = make_shared< CWCESpecularLayerFactory >( t_Material, t_Range );
      } else if( t_Material->Group == WindowBlind ) {
        aFactory = make_shared< CWCEVenetianBlindLayerFactory >( t_Material, t_Range );
      } else if( t_Material->Group == Screen ) {
        aFactory = make_shared< CWCEScreenLayerFactory >( t_Material, t_Range );
      } else if( t_Material->Group == Shade ) {
        aFactory = make_shared< CWCEDiffuseShadeLayerFactory >( t_Material, t_Range );
      }
      return aFactory->getLayer();
    }

    void InitWCE_BSDFOpticalData() {
      // SUBROUTINE INFORMATION:
      //       AUTHOR         Simon Vidanovic
      //       DATE WRITTEN   September 2016
      //       MODIFIED       na
      //       RE-ENGINEERED  na

      // PURPOSE OF THIS SUBROUTINE:
      // Initialize BSDF construction layers in Solar and Visible spectrum.

      CWindowConstructionsBSDF aWinConstBSDF = CWindowConstructionsBSDF::instance();
      for( auto ConstrNum = 1; ConstrNum <= TotConstructs; ++ConstrNum ) {
        auto& construction( Construct( ConstrNum ) );
        if( construction.isGlazingConstruction() ) {
          for( auto LayNum = 1; LayNum <= construction.TotLayers; ++LayNum ) {
            auto& material( Material( construction.LayerPoint( LayNum ) ) );
            if ( material.Group != WindowGas && material.Group != WindowGasMixture && 
              material.Group != ComplexWindowGap && material.Group != ComplexWindowShade ) {
              shared_ptr< MaterialProperties > aMaterial = make_shared< MaterialProperties >();
              *aMaterial = material;

              // This is necessary because rest of EnergyPlus code relies on TransDiff property 
              // of construction. It will basically trigger Window optical calculations if this
              // property is >0.
              construction.TransDiff = 0.1;

              auto aRange = WavelengthRange::Solar;
              shared_ptr< CBSDFLayer > aSolarLayer = getBSDFLayer( aMaterial, aRange );
              aWinConstBSDF.pushBSDFLayer( aRange, ConstrNum, aSolarLayer );

              aRange = WavelengthRange::Visible;
              shared_ptr< CBSDFLayer > aVisibleLayer = getBSDFLayer( aMaterial, aRange );
              aWinConstBSDF.pushBSDFLayer( aRange, ConstrNum, aVisibleLayer );
            }

          }
        }
      }
    }

    void InitWCE_SimplifiedOpticalData() {
      // SUBROUTINE INFORMATION:
      //       AUTHOR         Simon Vidanovic
      //       DATE WRITTEN   May 2017
      //       MODIFIED       na
      //       RE-ENGINEERED  na

      // PURPOSE OF THIS SUBROUTINE:
      // Initialize scattering construction layers in Solar and Visible spectrum.

      // Calculate optical properties of blind-type layers entered with MATERIAL:WindowBlind
		if ( TotBlinds > 0 ) CalcWindowBlindProperties();

		// Initialize SurfaceScreen structure
		if ( NumSurfaceScreens > 0 ) CalcWindowScreenProperties();

      CWindowConstructionsSimplified aWinConstSimp = CWindowConstructionsSimplified::instance();
      for( auto ConstrNum = 1; ConstrNum <= TotConstructs; ++ConstrNum ) {
        auto& construction( Construct( ConstrNum ) );
        if( construction.isGlazingConstruction() ) {
          for( auto LayNum = 1; LayNum <= construction.TotLayers; ++LayNum ) {
            auto& material( Material( construction.LayerPoint( LayNum ) ) );
            if( material.Group != WindowGas && material.Group != WindowGasMixture &&
              material.Group != ComplexWindowGap && material.Group != ComplexWindowShade ) {
              shared_ptr< MaterialProperties > aMaterial = make_shared< MaterialProperties >();
              *aMaterial = material;

              // This is necessary because rest of EnergyPlus code relies on TransDiff property 
              // of construction. It will basically trigger Window optical calculations if this
              // property is >0.
              construction.TransDiff = 0.1;

              auto aRange = WavelengthRange::Solar;
              shared_ptr< CScatteringLayer > aSolarLayer = getScatteringLayer( aMaterial, aRange );
              aWinConstSimp.pushLayer( aRange, ConstrNum, aSolarLayer );

              aRange = WavelengthRange::Visible;
              shared_ptr< CScatteringLayer > aVisibleLayer = getScatteringLayer( aMaterial, aRange );
              aWinConstSimp.pushLayer( aRange, ConstrNum, aVisibleLayer );
            }

          }
        }
      }

    // Get effective glass and shade/blind emissivities for windows that have interior blind or
		// shade. These are used to calculate zone MRT contribution from window when
		// interior blind/shade is deployed.

		for ( int SurfNum = 1; SurfNum <= TotSurfaces; ++SurfNum ) {
			if ( ! Surface( SurfNum ).HeatTransSurf ) continue;
			if ( ! Construct( Surface( SurfNum ).Construction ).TypeIsWindow ) continue;
			if ( SurfaceWindow( SurfNum ).WindowModelType == WindowBSDFModel ) continue; //Irrelevant for Complex Fen
			if ( Construct( Surface( SurfNum ).Construction ).WindowTypeEQL ) continue; // not required
			int ConstrNumSh = SurfaceWindow( SurfNum ).ShadedConstruction;
			if ( ConstrNumSh == 0 ) continue;
			int TotLay = Construct( ConstrNumSh ).TotLayers;
			bool IntShade = false;
			bool IntBlind = false;
      int ShadeLayPtr = 0;
      int BlNum = 0;
			if ( Material( Construct( ConstrNumSh ).LayerPoint( TotLay ) ).Group == Shade ) {
				IntShade = true;
				ShadeLayPtr = Construct( ConstrNumSh ).LayerPoint( TotLay );
			}
			if ( Material( Construct( ConstrNumSh ).LayerPoint( TotLay ) ).Group == WindowBlind ) {
				IntBlind = true;
				BlNum = Material( Construct( ConstrNumSh ).LayerPoint( TotLay ) ).BlindDataPtr;
			}

			if ( IntShade || IntBlind ) {
				for ( int ISlatAng = 1; ISlatAng <= MaxSlatAngs; ++ISlatAng ) {
          double EpsGlIR = 0;
          double RhoGlIR = 0;
					if ( IntShade || IntBlind ) {
						EpsGlIR = Material( Construct( ConstrNumSh ).LayerPoint( TotLay - 1 ) ).AbsorpThermalBack;
						RhoGlIR = 1 - EpsGlIR;
					}
					if ( IntShade ) {
						double TauShIR = Material( ShadeLayPtr ).TransThermal;
						double EpsShIR = Material( ShadeLayPtr ).AbsorpThermal;
						double RhoShIR = max( 0.0, 1.0 - TauShIR - EpsShIR );
						SurfaceWindow( SurfNum ).EffShBlindEmiss( 1 ) = EpsShIR * ( 1.0 + RhoGlIR * TauShIR / ( 1.0 - RhoGlIR * RhoShIR ) );
						SurfaceWindow( SurfNum ).EffGlassEmiss( 1 ) = EpsGlIR * TauShIR / ( 1.0 - RhoGlIR * RhoShIR );
					}
					if ( IntBlind ) {
						double TauShIR = Blind( BlNum ).IRFrontTrans( ISlatAng );
						double EpsShIR = Blind( BlNum ).IRBackEmiss( ISlatAng );
						double RhoShIR = max( 0.0, 1.0 - TauShIR - EpsShIR );
						SurfaceWindow( SurfNum ).EffShBlindEmiss( ISlatAng ) = EpsShIR * ( 1.0 + RhoGlIR * TauShIR / ( 1.0 - RhoGlIR * RhoShIR ) );
						SurfaceWindow( SurfNum ).EffGlassEmiss( ISlatAng ) = EpsGlIR * TauShIR / ( 1.0 - RhoGlIR * RhoShIR );
					}
					// Loop over remaining slat angles only if blind with movable slats
					if ( IntShade ) break; // Loop over remaining slat angles only if blind
					if ( IntBlind ) {
						if ( Blind( BlNum ).SlatAngleType == FixedSlats ) break;
					}
				} // End of slat angle loop
			} // End of check if interior shade or interior blind
		} // End of surface loop
    }

    ///////////////////////////////////////////////////////////////////////////////
    //   CWCEMaterialFactory
    ///////////////////////////////////////////////////////////////////////////////
    CWCEMaterialFactory::CWCEMaterialFactory( const shared_ptr< MaterialProperties >& t_Material,
      const WavelengthRange t_Range ) : 
      m_MaterialProperties( t_Material ), m_Range( t_Range ), m_Initialized( false ) {

    }

    shared_ptr< CMaterial > CWCEMaterialFactory::getMaterial() {
      if( !m_Initialized ) {
        init();
        m_Initialized = true;
      }
      return m_Material;
    }

    ///////////////////////////////////////////////////////////////////////////////
    //   CWCESpecularMaterialsFactory
    ///////////////////////////////////////////////////////////////////////////////
    CWCESpecularMaterialsFactory::CWCESpecularMaterialsFactory( 
      const shared_ptr< MaterialProperties >& t_Material,
      const WavelengthRange t_Range ) : CWCEMaterialFactory( t_Material, t_Range ) {
      
    }

    void CWCESpecularMaterialsFactory::init() {
      shared_ptr< CSeries > aSolarSpectrum = CWCESpecturmProperties::getDefaultSolarRadiationSpectrum();
      shared_ptr< CSpectralSampleData > aSampleData = nullptr;
      if( m_MaterialProperties->GlassSpectralDataPtr > 0 ) {
        aSampleData = CWCESpecturmProperties::getSpectralSample( m_MaterialProperties->GlassSpectralDataPtr );
      } else {
        aSampleData = CWCESpecturmProperties::getSpectralSample( *m_MaterialProperties );
      }
      
      shared_ptr< CSpectralSample > aSample = make_shared< CSpectralSample >( aSampleData, aSolarSpectrum );

      MaterialType aType = MaterialType::Monolithic;
      CWavelengthRange aRange = CWavelengthRange( m_Range );
      double lowLambda = aRange.minLambda();
      double highLambda = aRange.maxLambda();

      if( m_Range == WavelengthRange::Visible ) {
        shared_ptr< CSeries > aPhotopicResponse = CWCESpecturmProperties::getDefaultVisiblePhotopicResponse();
        aSample->setDetectorData( aPhotopicResponse );
      }

      double thickness = m_MaterialProperties->Thickness;
      m_Material = make_shared< CMaterialSample >( aSample, thickness, aType, lowLambda, highLambda );
    }

    ///////////////////////////////////////////////////////////////////////////////
    //   CWCEMaterialDualBandFactory
    ///////////////////////////////////////////////////////////////////////////////
    CWCEMaterialDualBandFactory::CWCEMaterialDualBandFactory( 
      const shared_ptr< MaterialProperties >& t_Material,
      const WavelengthRange t_Range ) : CWCEMaterialFactory( t_Material, t_Range ) {
      
    }

    void CWCEMaterialDualBandFactory::init() {
      if( m_Range == WavelengthRange::Visible ) {
        m_Material = createVisibleRangeMaterial();
      } else {
        shared_ptr< CMaterialSingleBand > aVisibleRangeMaterial = createVisibleRangeMaterial();
        shared_ptr< CMaterialSingleBand > aSolarRangeMaterial = createSolarRangeMaterial();
        // Ratio visible to solar range. It can be calculated from solar spectrum.
        double ratio = 0.49;
        m_Material = make_shared< CMaterialDualBand >( aVisibleRangeMaterial, aSolarRangeMaterial, ratio );
      }
    }

    ///////////////////////////////////////////////////////////////////////////////
    //   CWCEVenetianBlindMaterialsFactory
    ///////////////////////////////////////////////////////////////////////////////
    CWCEVenetianBlindMaterialsFactory::CWCEVenetianBlindMaterialsFactory( 
      const shared_ptr< MaterialProperties >& t_Material,
      const WavelengthRange t_Range ) : CWCEMaterialDualBandFactory( t_Material, t_Range ) {
      
    }

    shared_ptr< CMaterialSingleBand > CWCEVenetianBlindMaterialsFactory::createVisibleRangeMaterial() {
      int blindDataPtr = m_MaterialProperties->BlindDataPtr;
      auto& blind( Blind( blindDataPtr ) );
      assert( blindDataPtr > 0 );
      
      CWavelengthRange aRange = CWavelengthRange( WavelengthRange::Visible );
      double lowLambda = aRange.minLambda();
      double highLambda = aRange.maxLambda();
      
      double Tf = blind.SlatTransVisDiffDiff;
      double Tb = blind.SlatTransVisDiffDiff;
      double Rf = blind.SlatFrontReflVisDiffDiff;
      double Rb = blind.SlatBackReflVisDiffDiff;
      
      return make_shared< CMaterialSingleBand >( Tf, Tb, Rf, Rb, lowLambda, highLambda );
    }
    
    shared_ptr< CMaterialSingleBand > CWCEVenetianBlindMaterialsFactory::createSolarRangeMaterial() {
      int blindDataPtr = m_MaterialProperties->BlindDataPtr;
      auto& blind( Blind( blindDataPtr ) );
      assert( blindDataPtr > 0 );

      CWavelengthRange aRange = CWavelengthRange( WavelengthRange::Solar );
      double lowLambda = aRange.minLambda();
      double highLambda = aRange.maxLambda();

      double Tf = blind.SlatTransSolDiffDiff;
      double Tb = blind.SlatTransSolDiffDiff;
      double Rf = blind.SlatFrontReflSolDiffDiff;
      double Rb = blind.SlatBackReflSolDiffDiff;

      return make_shared< CMaterialSingleBand >( Tf, Tb, Rf, Rb, lowLambda, highLambda );
    }

    ///////////////////////////////////////////////////////////////////////////////
    //   CWCEScreenMaterialsFactory
    ///////////////////////////////////////////////////////////////////////////////
    CWCEScreenMaterialsFactory::CWCEScreenMaterialsFactory( const shared_ptr< MaterialProperties >& t_Material,
      const WavelengthRange t_Range ) : CWCEMaterialDualBandFactory( t_Material, t_Range ) {
      // Current EnergyPlus model does not support material transmittance different from zero.
      // To enable that, it would be necessary to change input in IDF
 
    }

    shared_ptr< CMaterialSingleBand > CWCEScreenMaterialsFactory::createVisibleRangeMaterial() {
      CWavelengthRange aRange = CWavelengthRange( WavelengthRange::Visible );
      double lowLambda = aRange.minLambda();
      double highLambda = aRange.maxLambda();

      double Tf = 0;
      double Tb = 0;
      double Rf = m_MaterialProperties->ReflectShadeVis;
      double Rb = m_MaterialProperties->ReflectShadeVis;

      return make_shared< CMaterialSingleBand >( Tf, Tb, Rf, Rb, lowLambda, highLambda );
    }

    shared_ptr< CMaterialSingleBand > CWCEScreenMaterialsFactory::createSolarRangeMaterial() {
      CWavelengthRange aRange = CWavelengthRange( WavelengthRange::Solar );
      double lowLambda = aRange.minLambda();
      double highLambda = aRange.maxLambda();

      double Tf = 0;
      double Tb = 0;
      double Rf = m_MaterialProperties->ReflectShade;
      double Rb = m_MaterialProperties->ReflectShade;

      return make_shared< CMaterialSingleBand >( Tf, Tb, Rf, Rb, lowLambda, highLambda );
    }

    ///////////////////////////////////////////////////////////////////////////////
    //   CWCEDiffuseShadeMaterialsFactory
    ///////////////////////////////////////////////////////////////////////////////
    CWCEDiffuseShadeMaterialsFactory::CWCEDiffuseShadeMaterialsFactory( const shared_ptr< MaterialProperties >& t_Material,
      const WavelengthRange t_Range ) : CWCEMaterialDualBandFactory( t_Material, t_Range ) {
      
    }

    shared_ptr< CMaterialSingleBand > CWCEDiffuseShadeMaterialsFactory::createVisibleRangeMaterial() {
      CWavelengthRange aRange = CWavelengthRange( WavelengthRange::Visible );
      double lowLambda = aRange.minLambda();
      double highLambda = aRange.maxLambda();

      double Tf = m_MaterialProperties->TransVis;
      double Tb = m_MaterialProperties->TransVis;
      double Rf = m_MaterialProperties->ReflectShadeVis;
      double Rb = m_MaterialProperties->ReflectShadeVis;

      return make_shared< CMaterialSingleBand >( Tf, Tb, Rf, Rb, lowLambda, highLambda );
    }

    shared_ptr< CMaterialSingleBand > CWCEDiffuseShadeMaterialsFactory::createSolarRangeMaterial() {
      CWavelengthRange aRange = CWavelengthRange( WavelengthRange::Solar );
      double lowLambda = aRange.minLambda();
      double highLambda = aRange.maxLambda();

      double Tf = m_MaterialProperties->Trans;
      double Tb = m_MaterialProperties->Trans;
      double Rf = m_MaterialProperties->ReflectShade;
      double Rb = m_MaterialProperties->ReflectShade;

      return make_shared< CMaterialSingleBand >( Tf, Tb, Rf, Rb, lowLambda, highLambda );
    }

    ///////////////////////////////////////////////////////////////////////////////
    //   CWCECellFactory
    ///////////////////////////////////////////////////////////////////////////////
    IWCECellDescriptionFactory::IWCECellDescriptionFactory( const shared_ptr< MaterialProperties >& t_Material ) :
      m_Material( t_Material ) {

    }

    ///////////////////////////////////////////////////////////////////////////////
    //   CWCESpecularCellFactory
    ///////////////////////////////////////////////////////////////////////////////
    CWCESpecularCellFactory::CWCESpecularCellFactory(
      const std::shared_ptr< EnergyPlus::DataHeatBalance::MaterialProperties >& t_Material ) :
      IWCECellDescriptionFactory( t_Material ) {

    }

    shared_ptr< ICellDescription > CWCESpecularCellFactory::getCellDescription() {
      return make_shared< CSpecularCellDescription >();
    }

    ///////////////////////////////////////////////////////////////////////////////
    //   CWCEVenetianBlindCellFactory
    ///////////////////////////////////////////////////////////////////////////////
    CWCEVenetianBlindCellFactory::CWCEVenetianBlindCellFactory(
      const std::shared_ptr< EnergyPlus::DataHeatBalance::MaterialProperties >& t_Material ) :
      IWCECellDescriptionFactory( t_Material ) {

    }

    shared_ptr< ICellDescription > CWCEVenetianBlindCellFactory::getCellDescription() {
      assert( m_Material != nullptr );
      int blindDataPtr = m_Material->BlindDataPtr;
      auto& blind( Blind( blindDataPtr ) );
      assert( blindDataPtr > 0 );

      double slatWidth = blind.SlatWidth;
      double slatSpacing = blind.SlatSeparation;
      double slatTiltAngle = blind.SlatAngle;
      double curvatureRadius = 0; // No curvature radius in current IDF definition
      size_t numOfSlatSegments = 5; // Number of segments to use in venetian calculations
      return make_shared< CVenetianCellDescription >( slatWidth, slatSpacing, slatTiltAngle,
        curvatureRadius, numOfSlatSegments );
    }

    ///////////////////////////////////////////////////////////////////////////////
    //   CWCEScreenCellFactory
    ///////////////////////////////////////////////////////////////////////////////
    CWCEScreenCellFactory::CWCEScreenCellFactory(
      const std::shared_ptr< EnergyPlus::DataHeatBalance::MaterialProperties >& t_Material ) :
      IWCECellDescriptionFactory( t_Material ) {

    }

    shared_ptr< ICellDescription > CWCEScreenCellFactory::getCellDescription() {
      assert( m_Material != nullptr );
      double diameter = m_Material->Thickness; // Thickness in this case is diameter
                                               // ratio is not saved withing material but rather calculated from transmittance
      double ratio = 1.0 - std::sqrt( m_Material->Trans );
      double spacing = diameter / ratio;
      return make_shared< CWovenCellDescription >( diameter, spacing );
    }

    ///////////////////////////////////////////////////////////////////////////////
    //   CWCEDiffuseShadeCellFactory
    ///////////////////////////////////////////////////////////////////////////////
    CWCEDiffuseShadeCellFactory::CWCEDiffuseShadeCellFactory(
      const std::shared_ptr< EnergyPlus::DataHeatBalance::MaterialProperties >& t_Material ) :
      IWCECellDescriptionFactory( t_Material ) {

    }

    shared_ptr< ICellDescription > CWCEDiffuseShadeCellFactory::getCellDescription() {
      return make_shared< CPerfectDiffuseCellDescription >();
    }

    ///////////////////////////////////////////////////////////////////////////////
    //   CWCEBSDFLayerFactory
    ///////////////////////////////////////////////////////////////////////////////
    CWCELayerFactory::CWCELayerFactory(
      const shared_ptr< MaterialProperties >& t_Material, const WavelengthRange t_Range ) : 
      m_Material( t_Material ), m_Range( t_Range ), m_BSDFInitialized( false ), 
      m_SimpleInitialized( false ), m_MaterialFactory( nullptr ) {
      
    }

    pair< shared_ptr< CMaterial >, shared_ptr< ICellDescription > > CWCELayerFactory::init() {
      createMaterialFactory();
      shared_ptr< CMaterial > aMaterial = m_MaterialFactory->getMaterial();
      assert( aMaterial != nullptr );
      shared_ptr< ICellDescription > aCellDescription = getCellDescription( );
      assert( aCellDescription != nullptr );
      
      return make_pair( aMaterial, aCellDescription );
    }

    shared_ptr< CBSDFLayer > CWCELayerFactory::getBSDFLayer() {
      if( !m_BSDFInitialized ) {
        pair< shared_ptr< CMaterial >, shared_ptr< ICellDescription > > res;
        res = init();
        shared_ptr< CBSDFHemisphere > aBSDF = make_shared< CBSDFHemisphere >( BSDFBasis::Full );
        
        CBSDFLayerMaker aMaker = CBSDFLayerMaker( res.first, aBSDF, res.second );
        m_BSDFLayer = aMaker.getLayer();
        m_BSDFInitialized = true;
      }
      return m_BSDFLayer;
    }

    shared_ptr< CScatteringLayer > CWCELayerFactory::getLayer() {
      if( !m_SimpleInitialized ) {
        pair< shared_ptr< CMaterial >, shared_ptr< ICellDescription > > res;
        res = init();

        m_ScatteringLayer = make_shared< CScatteringLayer >( res.first, res.second );
        m_SimpleInitialized = true;
      }
      return m_ScatteringLayer;
    }

    shared_ptr< ICellDescription > CWCELayerFactory::getCellDescription() {
      return m_CellFactory->getCellDescription();
    }

    ///////////////////////////////////////////////////////////////////////////////
    //   CWCESpecularLayerFactory
    ///////////////////////////////////////////////////////////////////////////////
    CWCESpecularLayerFactory::CWCESpecularLayerFactory( 
      const shared_ptr< MaterialProperties >& t_Material,
      const WavelengthRange t_Range ) : CWCELayerFactory( t_Material, t_Range ) {
      m_CellFactory = make_shared< CWCESpecularCellFactory >( t_Material );
    }

    void CWCESpecularLayerFactory::createMaterialFactory() {
      m_MaterialFactory = make_shared< CWCESpecularMaterialsFactory >( m_Material, m_Range );
    }

    ///////////////////////////////////////////////////////////////////////////////
    //   CWCEVenetianBlindLayerFactory
    ///////////////////////////////////////////////////////////////////////////////
    CWCEVenetianBlindLayerFactory::CWCEVenetianBlindLayerFactory( 
      const shared_ptr< MaterialProperties >& t_Material, const WavelengthRange t_Range ) : 
      CWCELayerFactory( t_Material, t_Range ) {
      m_CellFactory = make_shared< CWCEVenetianBlindCellFactory >( t_Material );
    }

    void CWCEVenetianBlindLayerFactory::createMaterialFactory() {
      m_MaterialFactory = make_shared< CWCEVenetianBlindMaterialsFactory >( m_Material, m_Range );
    }

    ///////////////////////////////////////////////////////////////////////////////
    //   CWCEScreenLayerFactory
    ///////////////////////////////////////////////////////////////////////////////
    CWCEScreenLayerFactory::CWCEScreenLayerFactory( 
      const shared_ptr< MaterialProperties >& t_Material, const WavelengthRange t_Range ) : 
      CWCELayerFactory( t_Material, t_Range ) {
      m_CellFactory = make_shared< CWCEScreenCellFactory >( t_Material );
    }

    void CWCEScreenLayerFactory::createMaterialFactory() {
      m_MaterialFactory = make_shared< CWCEScreenMaterialsFactory >( m_Material, m_Range );
    }

    ///////////////////////////////////////////////////////////////////////////////
    //   CWCEDiffuseShadeLayerFactory
    ///////////////////////////////////////////////////////////////////////////////
    CWCEDiffuseShadeLayerFactory::CWCEDiffuseShadeLayerFactory(
      const shared_ptr< MaterialProperties >& t_Material, const WavelengthRange t_Range ) :
      CWCELayerFactory( t_Material, t_Range ) {
      m_CellFactory = make_shared< CWCEDiffuseShadeCellFactory >( t_Material );
    }

    void CWCEDiffuseShadeLayerFactory::createMaterialFactory() {
      m_MaterialFactory = make_shared< CWCEDiffuseShadeMaterialsFactory >( m_Material, m_Range );
    }

  }
}