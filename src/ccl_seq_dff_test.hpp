#ifndef _CCL_SEQ_DFF_TEST_HPP_
#define _CCL_SEQ_DFF_TEST_HPP_

#include <stdint.h>
#include "samc21.h"
#include "serial.hpp"
#include "timebase.hpp"
#include "bytestream.hpp"

// noDFF design open verifications §16.2 and §16.3, RESOLVED on bench
// 2026-05-21.
//
//   §16.3 — With LUT3 configured FILTSEL=SYNCH + EDGESEL=1, SEQ1 DFF
//           latches D on the rising edge of LUT_odd (LUT3) input.  PASS.
//           Without those bits the SEQ block degenerates to a gated
//           latch (Q transparent while G high) — see the first iteration
//           of this test for the latch-mode evidence.
//   §16.2 — INSEL=LINK on LUT_n reads the post-SEQ output of LUT_{n+1}
//           (Q in DFF mode), not the raw LUT_{n+1} truth (pre-SEQ D).
//           PASS in both edge and latch SEQ configurations.
//
// CCL layout:
//   LUT1: IN0=LINK (reads LUT2.OUT after SEQ1 substitution = Q),
//         TRUTH=0xAA (passes IN0).  LUT1 output drives PA11 = `CCL_OUT1`.
//   LUT2: IN0=IO (PA22 = `CCL_IN6`), TRUTH=0xAA (D = IN0).  SEQ1 D-input.
//   LUT3: IN0=IO (PB14 = `CCL_IN9`), TRUTH=0xAA (CLK = IN0).  SEQ1 CLK-input.
//   SEQCTRL[1] = SEQSEL_DFF.  SEQCTRL[0] = 0.
//   LUT2 output is replaced by Q at every consumer (pad PA25, LINK to LUT1).
//
// Pin pull-resistor strategy (same as the duty test): D and CLK pins stay
// muxed as CCL inputs; PINCFG.PULLEN + PORT.OUT control the pull direction.
// Transitions take ~1 us — slow but monotonic, so the LUT3 edge detector
// (FILTSEL=SYNCH + EDGESEL=1) sees a single clean rising edge.  Each phase
// is held for 500 us to give the pull (and the DFF) ample time to settle.
//
// IMPORTANT: enabling FILTSEL+EDGESEL on LUT_odd (the CLK source) is what
// makes the SEQ "DFF" actually behave as a rising-edge flip-flop.  Without
// these the same SEQSEL=DFF setting degenerates to a transparent latch
// (gated by LUT_odd as a level), since the SEQ block's "G" input is the
// post-filter post-edge-detector LUT_odd output.  Datasheet §37.6.2.7 and
// §37.6.2.6 read together make this explicit.
//
// Pad readouts: PA25 = Q (LUT2 post-SEQ), PA11 = LINK-passthrough through
// LUT1.  Both pads with INEN=1 are readable via PORT.IN.
//
// External hardware: none.  No jumpers.  SWD intact.

namespace CclSeqDffTest {

static constexpr uint32_t SETTLE_US = 500u;

static inline void wait_us(uint32_t us)
{
    uint32_t start = Timebase::micros();
    while ((Timebase::micros() - start) < us)
        ;
}

static inline void d_set(bool v)
{
    if (v) PORT->Group[0].OUTSET.reg = (1ul << 22);
    else   PORT->Group[0].OUTCLR.reg = (1ul << 22);
}

static inline void clk_set(bool v)
{
    if (v) PORT->Group[1].OUTSET.reg = (1ul << 14);
    else   PORT->Group[1].OUTCLR.reg = (1ul << 14);
}

static inline bool q_read(void)
{
    return (PORT->Group[0].IN.reg & (1ul << 25)) != 0u;
}

static inline bool link_read(void)
{
    return (PORT->Group[0].IN.reg & (1ul << 11)) != 0u;
}

struct Sample {
    const char* phase;
    bool d, clk;
    bool q, link;
    bool q_expected;          // model says Q should be this
    bool link_if_post_seq;    // if LINK reads Q, expected LINK value
    bool link_if_pre_seq;     // if LINK reads pre-SEQ D, expected LINK value
};

static inline void take(Sample& s, const char* name, bool d, bool clk,
                        bool q_expected, bool link_if_post_seq,
                        bool link_if_pre_seq)
{
    wait_us(SETTLE_US);
    s.phase             = name;
    s.d                 = d;
    s.clk               = clk;
    s.q                 = q_read();
    s.link              = link_read();
    s.q_expected        = q_expected;
    s.link_if_post_seq  = link_if_post_seq;
    s.link_if_pre_seq   = link_if_pre_seq;
}

static inline void run(void)
{
    Serial.print("\r\nCCL SEQ1 DFF + INSEL=LINK semantics test (noDFF §16.2, §16.3)");
    Serial.newline();
    Serial.print("  D   on PA22 (CCL_IN6 -> LUT2/IN0) via internal pull");
    Serial.newline();
    Serial.print("  CLK on PB14 (CCL_IN9 -> LUT3/IN0) via internal pull");
    Serial.newline();
    Serial.print("  Q   on PA25 (CCL_OUT2 = LUT2 post-SEQ1)");
    Serial.newline();
    Serial.print("  LNK on PA11 (CCL_OUT1, LUT1 INSEL0=LINK reading LUT2.OUT)");
    Serial.newline();

    MCLK->APBCMASK.reg |= MCLK_APBCMASK_CCL;
    GCLK->PCHCTRL[CCL_GCLK_ID].reg = GCLK_PCHCTRL_GEN_GCLK0 | GCLK_PCHCTRL_CHEN;
    while (!(GCLK->PCHCTRL[CCL_GCLK_ID].reg & GCLK_PCHCTRL_CHEN))
        ;

    // PA22 mux I = CCL_IN6 (D), INEN=1, PULLEN=1.  OUT bit picks pull dir.
    PORT->Group[0].DIRCLR.reg = (1ul << 22);
    PORT->Group[0].OUTCLR.reg = (1ul << 22);
    PORT->Group[0].PINCFG[22].reg =
        PORT_PINCFG_PMUXEN | PORT_PINCFG_INEN | PORT_PINCFG_PULLEN;
    PORT->Group[0].PMUX[11].bit.PMUXE = MUX_PA22I_CCL_IN6;

    // PB14 mux I = CCL_IN9 (CLK), INEN=1, PULLEN=1.
    PORT->Group[1].DIRCLR.reg = (1ul << 14);
    PORT->Group[1].OUTCLR.reg = (1ul << 14);
    PORT->Group[1].PINCFG[14].reg =
        PORT_PINCFG_PMUXEN | PORT_PINCFG_INEN | PORT_PINCFG_PULLEN;
    PORT->Group[1].PMUX[7].bit.PMUXE = MUX_PB14I_CCL_IN9;

    // PA25 mux I = CCL_OUT2.  INEN=1 so PORT.IN reflects Q.
    PORT->Group[0].PINCFG[25].reg = PORT_PINCFG_PMUXEN | PORT_PINCFG_INEN;
    PORT->Group[0].PMUX[12].bit.PMUXO = MUX_PA25I_CCL_OUT2;

    // PA11 mux I = CCL_OUT1.  INEN=1 so PORT.IN reflects LUT1 output.
    PORT->Group[0].PINCFG[11].reg = PORT_PINCFG_PMUXEN | PORT_PINCFG_INEN;
    PORT->Group[0].PMUX[5].bit.PMUXO = MUX_PA11I_CCL_OUT1;

    // CCL configuration.  All LUTCTRLn + SEQCTRLn written with CTRL.ENABLE=0
    // (errata DS80000740S 1.7.3).  No SWRST (errata 1.7.4).
    CCL->CTRL.reg = 0;
    CCL->SEQCTRL[0].reg = 0;
    CCL->SEQCTRL[1].reg = CCL_SEQCTRL_SEQSEL_DFF;

    CCL->LUTCTRL[0].reg = 0;

    // LUT1: TRUTH=0xAA, IN0=LINK (reads LUT2.OUT after SEQ1 = Q),
    // IN1/IN2 = MASK.  Output to PA11 pad.  LUTEO=0 (no EVSYS).
    CCL->LUTCTRL[1].reg = CCL_LUTCTRL_TRUTH(0xAAu)
                       | CCL_LUTCTRL_INSEL0(CCL_LUTCTRL_INSEL0_LINK_Val)
                       | CCL_LUTCTRL_INSEL1(CCL_LUTCTRL_INSEL0_MASK_Val)
                       | CCL_LUTCTRL_INSEL2(CCL_LUTCTRL_INSEL0_MASK_Val)
                       | CCL_LUTCTRL_ENABLE;

    // LUT2 (SEQ1 D): TRUTH=0xAA passes IN0 from PA22.
    CCL->LUTCTRL[2].reg = CCL_LUTCTRL_TRUTH(0xAAu)
                       | CCL_LUTCTRL_INSEL0(CCL_LUTCTRL_INSEL0_IO_Val)
                       | CCL_LUTCTRL_INSEL1(CCL_LUTCTRL_INSEL0_MASK_Val)
                       | CCL_LUTCTRL_INSEL2(CCL_LUTCTRL_INSEL0_MASK_Val)
                       | CCL_LUTCTRL_ENABLE;

    // LUT3 (SEQ1 CLK): TRUTH=0xAA passes IN0 from PB14.
    // FILTSEL=SYNCH (2-cycle synchronizer) + EDGESEL=1 turn LUT3 output into
    // a 1-cycle GCLK_CCL strobe per rising edge of PB14.  This is what makes
    // the SEQ "DFF" behave as a true rising-edge flip-flop instead of as a
    // gated latch (datasheet §37.6.2.6, §37.6.2.7).
    CCL->LUTCTRL[3].reg = CCL_LUTCTRL_TRUTH(0xAAu)
                       | CCL_LUTCTRL_INSEL0(CCL_LUTCTRL_INSEL0_IO_Val)
                       | CCL_LUTCTRL_INSEL1(CCL_LUTCTRL_INSEL0_MASK_Val)
                       | CCL_LUTCTRL_INSEL2(CCL_LUTCTRL_INSEL0_MASK_Val)
                       | CCL_LUTCTRL_FILTSEL_SYNCH
                       | CCL_LUTCTRL_EDGESEL
                       | CCL_LUTCTRL_ENABLE;

    CCL->CTRL.reg = CCL_CTRL_ENABLE;

    // Force a known initial Q by holding D=0, then a clean CLK rise.
    d_set(false);
    clk_set(false);

    Sample s[9];

    take(s[0], "0 init           D=0 CLK=0",
         false, false, /*Q*/ false, /*L=Q*/ false, /*L=D*/ false);
    clk_set(true);
    take(s[1], "1 CLK rise D=0           D=0 CLK=1 -> Q latches 0",
         false, true,  /*Q*/ false, /*L=Q*/ false, /*L=D*/ false);

    d_set(true);
    take(s[2], "2 D=1, CLK held high     D=1 CLK=1 -> Q unchanged",
         true,  true,  /*Q*/ false, /*L=Q*/ false, /*L=D*/ true);

    clk_set(false);
    take(s[3], "3 CLK fall, D=1          D=1 CLK=0 -> Q unchanged",
         true,  false, /*Q*/ false, /*L=Q*/ false, /*L=D*/ true);

    // Oscillate D while CLK is low — Q must not move.
    d_set(false); wait_us(SETTLE_US);
    d_set(true);  wait_us(SETTLE_US);
    d_set(false); wait_us(SETTLE_US);
    d_set(true);
    take(s[4], "4 D oscillates, CLK=0    D=1 CLK=0 -> Q unchanged",
         true,  false, /*Q*/ false, /*L=Q*/ false, /*L=D*/ true);

    clk_set(true);
    take(s[5], "5 CLK rise D=1           D=1 CLK=1 -> Q latches 1",
         true,  true,  /*Q*/ true,  /*L=Q*/ true,  /*L=D*/ true);

    d_set(false);
    take(s[6], "6 D=0, CLK held high     D=0 CLK=1 -> Q unchanged",
         false, true,  /*Q*/ true,  /*L=Q*/ true,  /*L=D*/ false);

    clk_set(false);
    take(s[7], "7 CLK fall, D=0          D=0 CLK=0 -> Q unchanged",
         false, false, /*Q*/ true,  /*L=Q*/ true,  /*L=D*/ false);

    clk_set(true);
    take(s[8], "8 CLK rise D=0           D=0 CLK=1 -> Q latches 0",
         false, true,  /*Q*/ false, /*L=Q*/ false, /*L=D*/ false);

    auto bit = [](bool b) -> uint32_t { return b ? 1u : 0u; };

    for (uint32_t i = 0; i < 9; ++i) {
        const Sample& x = s[i];
        Serial.print("  ");
        Serial.print(x.phase);
        Serial.print("   Q=");
        Serial.print(bit(x.q));
        Serial.print(" (exp ");
        Serial.print(bit(x.q_expected));
        Serial.print(")  LNK=");
        Serial.print(bit(x.link));
        Serial.print(" (postSEQ=");
        Serial.print(bit(x.link_if_post_seq));
        Serial.print(" preSEQ=");
        Serial.print(bit(x.link_if_pre_seq));
        Serial.print(")");
        Serial.newline();
    }

    // Verdicts.
    uint32_t dff_mismatches  = 0;
    uint32_t link_eq_q       = 0;
    uint32_t link_eq_d       = 0;
    uint32_t link_eq_neither = 0;
    uint32_t discrim_steps   = 0;   // step 2 and step 6 — Q != D
    uint32_t discrim_q_wins  = 0;
    uint32_t discrim_d_wins  = 0;

    for (uint32_t i = 0; i < 9; ++i) {
        const Sample& x = s[i];
        if (x.q != x.q_expected)
            ++dff_mismatches;
        if (x.link == x.link_if_post_seq)
            ++link_eq_q;
        if (x.link == x.link_if_pre_seq)
            ++link_eq_d;
        if (x.link != x.link_if_post_seq && x.link != x.link_if_pre_seq)
            ++link_eq_neither;
        if (x.link_if_post_seq != x.link_if_pre_seq) {
            ++discrim_steps;
            if (x.link == x.link_if_post_seq) ++discrim_q_wins;
            if (x.link == x.link_if_pre_seq)  ++discrim_d_wins;
        }
    }

    Serial.print("Stats: DFF mismatches=");
    Serial.print(dff_mismatches);
    Serial.print("/9  LNK==postSEQ=");
    Serial.print(link_eq_q);
    Serial.print("/9  LNK==preSEQ=");
    Serial.print(link_eq_d);
    Serial.print("/9  neither=");
    Serial.print(link_eq_neither);
    Serial.newline();
    Serial.print("Discriminator steps (2 and 6): postSEQ matches=");
    Serial.print(discrim_q_wins);
    Serial.print("/");
    Serial.print(discrim_steps);
    Serial.print("  preSEQ matches=");
    Serial.print(discrim_d_wins);
    Serial.print("/");
    Serial.print(discrim_steps);
    Serial.newline();

    bool dff_ok      = (dff_mismatches == 0);
    bool link_is_q   = (link_eq_q == 9);
    bool link_is_d   = (link_eq_d == 9);

    Serial.print("VERDICT: ");
    if (dff_ok && link_is_q) {
        Serial.print("OK -- DFF latches on CLK rising; LINK reads post-SEQ Q");
        Serial.newline();
        Serial.print("         noDFF design §16.2 and §16.3 both pass");
    } else if (dff_ok && link_is_d) {
        Serial.print("DFF OK but LINK reads pre-SEQ D (LUT_even truth)");
        Serial.newline();
        Serial.print("         §16.3 passes; §16.2 fails -- Q must reach LUT1 via EVSYS, not LINK");
    } else if (!dff_ok) {
        Serial.print("DFF BROKEN -- Q does not latch correctly");
        Serial.newline();
        Serial.print("         noDFF design abandoned -- inspect step-by-step output above");
    } else {
        Serial.print("ANOMALY -- LINK matches neither post-SEQ Q nor pre-SEQ D consistently");
    }
    Serial.newline();
}

} // namespace CclSeqDffTest

#endif // _CCL_SEQ_DFF_TEST_HPP_
