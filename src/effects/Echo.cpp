/**********************************************************************

  Audacity: A Digital Audio Editor

  Echo.cpp

  Dominic Mazzoni
  Vaughan Johnson (dialog)

*******************************************************************//**

\class EffectEcho
\brief An Effect that causes an echo, variable delay and volume.

*//****************************************************************//**

\class EchoDialog
\brief EchoDialog used with EffectEcho

*//*******************************************************************/


#include "Echo.h"
#include "LoadEffects.h"

#include <float.h>

#include <wx/intl.h>

#include "../ShuttleGui.h"
#include "../Shuttle.h"
#include "../widgets/AudacityMessageBox.h"
#include "../widgets/valnum.h"

namespace {
EffectParameter Delay{ &EffectEcho::delay,
   L"Delay",   1.0f, 0.001f,  FLT_MAX, 1.0f };
EffectParameter Decay{ &EffectEcho::decay,
   L"Decay",   0.5f, 0.0f,    FLT_MAX, 1.0f };
}

const ComponentInterfaceSymbol EffectEcho::Symbol
{ XO("Echo") };

namespace{ BuiltinEffectsModule::Registration< EffectEcho > reg; }

EffectEcho::EffectEcho()
{
   delay = Delay.def;
   decay = Decay.def;

   SetLinearEffectFlag(true);
}

EffectEcho::~EffectEcho()
{
}

// ComponentInterface implementation

ComponentInterfaceSymbol EffectEcho::GetSymbol() const
{
   return Symbol;
}

TranslatableString EffectEcho::GetDescription() const
{
   return XO("Repeats the selected audio again and again");
}

ManualPageID EffectEcho::ManualPage() const
{
   return L"Echo";
}

// EffectDefinitionInterface implementation

EffectType EffectEcho::GetType() const
{
   return EffectTypeProcess;
}

// EffectProcessor implementation

unsigned EffectEcho::GetAudioInCount() const
{
   return 1;
}

unsigned EffectEcho::GetAudioOutCount() const
{
   return 1;
}

bool EffectEcho::ProcessInitialize(
   EffectSettings &, sampleCount, ChannelNames)
{
   if (delay == 0.0)
   {
      return false;
   }

   histPos = 0;
   auto requestedHistLen = (sampleCount) (mSampleRate * delay);

   // Guard against extreme delay values input by the user
   try {
      // Guard against huge delay values from the user.
      // Don't violate the assertion in as_size_t
      if (requestedHistLen !=
            (histLen = static_cast<size_t>(requestedHistLen.as_long_long())))
         throw std::bad_alloc{};
      history.reinit(histLen, true);
   }
   catch ( const std::bad_alloc& ) {
      Effect::MessageBox( XO("Requested value exceeds memory capacity.") );
      return false;
   }

   return history != NULL;
}

bool EffectEcho::ProcessFinalize()
{
   history.reset();
   return true;
}

size_t EffectEcho::ProcessBlock(EffectSettings &,
   const float *const *inBlock, float *const *outBlock, size_t blockLen)
{
   const float *ibuf = inBlock[0];
   float *obuf = outBlock[0];

   for (decltype(blockLen) i = 0; i < blockLen; i++, histPos++)
   {
      if (histPos == histLen)
      {
         histPos = 0;
      }
      history[histPos] = obuf[i] = ibuf[i] + history[histPos] * decay;
   }

   return blockLen;
}

bool EffectEcho::VisitSettings( SettingsVisitor & S ){
   S.SHUTTLE_PARAM( delay, Delay );
   S.SHUTTLE_PARAM( decay, Decay );
   return true;
}


bool EffectEcho::GetAutomationParameters(CommandParameters & parms) const
{
   parms.WriteFloat(Delay.key, delay);
   parms.WriteFloat(Decay.key, decay);

   return true;
}

bool EffectEcho::SetAutomationParameters(const CommandParameters & parms)
{
   ReadParam(Delay);
   ReadParam(Decay);

   delay = Delay;
   decay = Decay;

   return true;
}

std::unique_ptr<EffectUIValidator>
EffectEcho::PopulateOrExchange(ShuttleGui & S, EffectSettingsAccess &)
{
   S.AddSpace(0, 5);

   S.StartMultiColumn(2, wxALIGN_CENTER);
   {
      S.Validator<FloatingPointValidator<double>>(
            3, &delay, NumValidatorStyle::NO_TRAILING_ZEROES,
            Delay.min, Delay.max )
         .AddTextBox(XXO("&Delay time (seconds):"), L"", 10);

      S.Validator<FloatingPointValidator<double>>(
            3, &decay, NumValidatorStyle::NO_TRAILING_ZEROES,
            Decay.min, Decay.max)
         .AddTextBox(XXO("D&ecay factor:"), L"", 10);
   }
   S.EndMultiColumn();
   return nullptr;
}
