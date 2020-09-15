#include "ChannelClient.h"

// objects Includes
#include <Expertise.h>
#include <StatusEffect.h>

// Google Test Includes
#include <PopIgnore.h>
#include <PushIgnore.h>
#include <gtest/gtest.h>

using namespace libtester;

void ChannelClient::HandleCharacterData(libcomp::ReadOnlyPacket& p) {
  mEntityID = p.ReadS32Little();
  mCharacter->SetName(p.ReadString16Little(libcomp::Convert::ENCODING_CP932));
  p.ReadU32Little();  // demon title
  mCharacter->SetGender(static_cast<objects::Character::Gender_t>(p.ReadU8()));
  mCharacter->SetSkinType(p.ReadU8());
  mCharacter->SetHairType(p.ReadU8());
  mCharacter->SetHairColor(p.ReadU8());
  mCharacter->SetEyeType(p.ReadU8());
  mCharacter->SetRightEyeColor(p.ReadU8());
  mCharacter->SetFaceType(p.ReadU8());
  mCharacter->SetLeftEyeColor(p.ReadU8());
  p.ReadU8();  // unknown
  p.ReadU8();  // unknown bool

  // uint32_t equippedItems[15];

  for (size_t i = 0; i < 15; i++) {
    // equippedItems[i] = p.ReadU32Little();
    p.ReadU32Little();
  }

  auto cs = mCharacter->GetCoreStats();
  cs->SetMaxHP(p.ReadS16Little());
  cs->SetMaxMP(p.ReadS16Little());
  cs->SetHP(p.ReadS16Little());
  cs->SetMP(p.ReadS16Little());
  cs->SetXP(p.ReadS64Little());
  mCharacter->SetPoints(p.ReadS32Little());
  cs->SetLevel(p.ReadS8());
  mCharacter->SetLNC(p.ReadS16Little());

  cs->SetSTR(p.ReadS16Little());
  cs->SetSTR(p.ReadS16Little());
  cs->SetMAGIC(p.ReadS16Little());
  cs->SetMAGIC(p.ReadS16Little());
  cs->SetVIT(p.ReadS16Little());
  cs->SetVIT(p.ReadS16Little());
  cs->SetINTEL(p.ReadS16Little());
  cs->SetINTEL(p.ReadS16Little());
  cs->SetSPEED(p.ReadS16Little());
  cs->SetSPEED(p.ReadS16Little());
  cs->SetLUCK(p.ReadS16Little());
  cs->SetLUCK(p.ReadS16Little());

  cs->SetCLSR(p.ReadS16Little());
  cs->SetCLSR(p.ReadS16Little());
  cs->SetLNGR(p.ReadS16Little());
  cs->SetLNGR(p.ReadS16Little());
  cs->SetSPELL(p.ReadS16Little());
  cs->SetSPELL(p.ReadS16Little());
  cs->SetSUPPORT(p.ReadS16Little());
  cs->SetSUPPORT(p.ReadS16Little());
  cs->SetPDEF(p.ReadS16Little());
  cs->SetPDEF(p.ReadS16Little());
  cs->SetMDEF(p.ReadS16Little());
  cs->SetMDEF(p.ReadS16Little());

  p.ReadS16Little();  // unknown
  p.ReadS16Little();  // unknown

  auto statusEffectCount = p.ReadU32Little();

  for (uint32_t i = 0; i < statusEffectCount; ++i) {
    auto effect = std::make_shared<objects::StatusEffect>();
    effect->SetEffect(p.ReadU32Little());
    effect->SetExpiration(p.ReadU32Little());
    effect->SetStack(p.ReadU8());

    mCharacter->AppendStatusEffects(effect);
  }

  auto currentSkillsCount = p.ReadU32Little();

  for (uint32_t i = 0; i < currentSkillsCount; ++i) {
    mCharacter->InsertLearnedSkills(p.ReadU32Little());
  }

  for (size_t i = 0; i < 38; i++) {
    auto expertise = std::make_shared<objects::Expertise>();
    expertise->SetPoints(p.ReadS32Little());
    expertise->SetExpertiseID(p.ReadU8());
    expertise->SetDisabled(0 != p.ReadU8());

    mCharacter->SetExpertises(i, expertise);
  }

  p.ReadU8();  // unknown bool
  p.ReadU8();  // unknown bool
  p.ReadU8();  // unknown bool
  p.ReadU8();  // unknown bool

  auto activeDemon = p.ReadS64Little();
  (void)activeDemon;
  /// @todo Do something with it...

  p.ReadS64Little();  // unknown
  p.ReadS64Little();  // unknown

  EXPECT_EQ(0, p.Left());
}
