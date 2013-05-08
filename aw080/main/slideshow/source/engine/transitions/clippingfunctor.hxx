/**************************************************************
 * 
 * Licensed to the Apache Software Foundation (ASF) under one
 * or more contributor license agreements.  See the NOTICE file
 * distributed with this work for additional information
 * regarding copyright ownership.  The ASF licenses this file
 * to you under the Apache License, Version 2.0 (the
 * "License"); you may not use this file except in compliance
 * with the License.  You may obtain a copy of the License at
 * 
 *   http://www.apache.org/licenses/LICENSE-2.0
 * 
 * Unless required by applicable law or agreed to in writing,
 * software distributed under the License is distributed on an
 * "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
 * KIND, either express or implied.  See the License for the
 * specific language governing permissions and limitations
 * under the License.
 * 
 *************************************************************/



#if ! defined INCLUDED_SLIDESHOW_CLIPPINGFUNCTOR_HXX
#define INCLUDED_SLIDESHOW_CLIPPINGFUNCTOR_HXX

#include <basegfx/numeric/ftools.hxx>
#include <basegfx/vector/b2dsize.hxx>
#include <basegfx/matrix/b2dhommatrix.hxx>
#include <basegfx/polygon/b2dpolypolygontools.hxx>
#include <transitioninfo.hxx>
#include <parametricpolypolygon.hxx>


namespace slideshow 
{
    namespace internal 
    {
        /** Generates the final clipping polygon.

        	This class serves as the functor, which generates the
        	final clipping polygon from a given ParametricPolyPolygon
        	and a TransitionInfo. 

            The ParametricPolyPolygon can be obtained from the
            ParametricPolyPolygonFactory, see there. 

            The TransitionInfo further parameterizes the polygon
            generated by the ParametricPolyPolygon, with common
            modifications such as rotation, flipping, or change of
            direction. This allows the ParametricPolyPolygonFactory to
            provide only prototypical shapes, with the ClippingFunctor
            further customizing the output.
         */
        class ClippingFunctor
        {
        public:
            ClippingFunctor(
                const ParametricPolyPolygonSharedPtr&   rPolygon,
                const TransitionInfo&                   rTransitionInfo,
                bool                                    bDirectionForward,
                bool                                    bModeIn );
            
            /** Generate clip polygon.

            	@param nValue
                Value to generate the polygon for. Must be in the
                range [0,1].

                @param rTargetSize
                Size the clip polygon should cover. This is typically
                the size of the object the effect is applied on.
             */
            ::basegfx::B2DPolyPolygon operator()( double 					nValue,
                                                  const ::basegfx::B2DSize& rTargetSize );
        
        private:
            ParametricPolyPolygonSharedPtr     mpParametricPoly;
            ::basegfx::B2DHomMatrix            maStaticTransformation;
            // AW: Not needed
			// ::basegfx::B2DPolyPolygon          maBackgroundRect;
            bool                               mbForwardParameterSweep;
            bool                               mbSubtractPolygon;
            const bool                         mbScaleIsotrophically;
            bool                               mbFlip;
        };

    }
}

#endif /* INCLUDED_SLIDESHOW_CLIPPINGFUNCTOR_HXX */