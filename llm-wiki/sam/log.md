# Wiki Log

## [2026-05-06] experiment | LUT2/TCC2 INSEL1+INSEL2 confirmed — mapping 100% complete

- LUT2 INSEL1=TCC → 75% (WO[1]) ✓; INSEL2=TCC → 25% (WO[2]=WO[0], 2 CC only) ✓.
- All 4 LUTs, all 3 INSEL slots now exhaustively verified by scope.
- Pages updated: [[CCL Configuration]]

## [2026-05-06] experiment | LUT3 all 3 INSEL slots confirmed (TCC0 wraparound)

- LUT3 INSEL1=TCC → 75% (WO[1]) ✓; INSEL2=TCC → 50% (WO[2]) ✓.
- LUT3 behaves identically to LUT0: TCC0, INSEL_n→WO[n].
- Full INSEL=TCC mapping on SAMC21J18A now exhaustively verified by scope.
- Pages updated: [[CCL Configuration]]

## [2026-05-06] experiment | LUT3 wraparound confirmed — complete INSEL=TCC mapping established

- LUT3 INSEL0=TCC(0x8) with TCC0 running: PB17 shows 25% = TCC0 WO[0]. Wraparound confirmed.
- No TCC3 on J18A → LUT3 wraps back to TCC0, same as LUT0.
- **Complete mapping: LUT0→TCC0, LUT1→TCC1, LUT2→TCC2, LUT3→TCC0 (wrap).**
- **INSEL_n=TCC → WO[n] confirmed universally across all tested LUTs.**
- Pages updated: [[CCL Configuration]]

## [2026-05-06] experiment | TCC0→LUT0 confirmed; INSEL_n→WO[n] law universally verified

- TCC0 NPWM: WO[0]=25%, WO[1]=75%, WO[2]=50%. LUT0 INSEL slot isolated per test.
- INSEL0→25%, INSEL1→75%, INSEL2→50% on PA07 (LUT0 OUT). All match WO index.
- Combined with TCC1/LUT1 results: **INSEL_n=TCC routes WO[n]** confirmed for TCC0 and TCC1.
- Combined with TCC_n→LUT_n: TCC0→LUT0, TCC1→LUT1, TCC2→LUT2 all confirmed.
- Previous "TCC0→LUT0 disproved" was measurement error (wrong PMUX + TRUTH bug).
- Pages updated: [[CCL Configuration]]

## [2026-05-06] experiment | TCC2→LUT2 confirmed by scope; EVSYS data discarded

- TCC2 NPWM CC[0]=25% on PA12; LUT2 INSEL0=TCC(0x8), TRUTH=0x02; LUT2 OUT on PA25.
- PA25 shows 25% → TCC2→LUT2 confirmed. Pattern: TCC_n→LUT_n (n=1,2).
- TCC0→LUT0 and TCC0→LUT3 disproved (flat scope). EVSYS experiment results discarded.
- TCC0 routing via INSEL=TCC remains unresolved.
- Pages updated: [[CCL Configuration]]

## [2026-05-06] experiment | CCL INSEL slot → WO channel mapping confirmed (TCC1/LUT1)

- Method: isolated INSEL slot test — one slot=TCC(0x8), others=MASK(0x0).
  TCC1 NPWM CC[0]=25%, CC[1]=75%. LUT1 OUT on PA11.
- INSEL0 → WO[0] (25% ✓), INSEL1 → WO[1] (75% ✓), INSEL2 → WO[2]=WO[0] (25% ✓).
- **INSEL_n = TCC routes WO[n] to that LUT input.** Slot index = WO index.
- Consequence: AC?WO0:WO1 mux achievable with zero PCB loopback pins (INSEL0=TCC/WO0, INSEL1=TCC/WO1, INSEL2=AC, TRUTH=0xCA).
- Prior claim "all slots receive WO[0]" in wiki was wrong — corrected.
- Pages updated: [[CCL Configuration]]

## [2026-05-06] experiment | CCL INSEL=TCC — TCC1→LUT1 confirmed by oscilloscope

- Method: isolated scope test — PA06=TCC1 WO0 reference, PA11=CCL LUT1 OUT.
- TCC1 NPWM 100 Hz 50% duty; CCL LUT1 INSEL0=TCC(0x8), TRUTH=0x02 (pass-through).
- Result: PA11 == PA06, same phase. **INSEL=TCC (0x8) is definitively functional on SAMC21J18A.**
- No EVSYS/TC chain involved — highest confidence result.
- Previous EVSYS experiment data (TCC0→LUT0/LUT3, TCC2→LUT1) marked preliminary pending clean re-test.
- Pages updated: [[CCL Configuration]]

## [2026-05-06] experiment | CCL INSEL=TCC mapping fully verified (systematic test)

- Method: 3 TCCs × 3 INSEL slots × 4 LUTs; EVSYS pulse-counting via TC EVACT=COUNT.
- **TCC0 → LUT0 and LUT3** (all three INSEL slots); TCC1 → LUT1; TCC2 → LUT2.
- INSEL slot (IN[0]/IN[1]/IN[2]) does NOT select a different WO channel — all slots
  receive the same TCC WO[0]. Two different WO outputs cannot both enter the same LUT
  via INSEL=TCC; WO[1] requires INSEL=IO + physical pin.
- TCC0 covers LUT3 because TCC3/TCC4 (N-variant) are absent on J18A.
- Design implication: `WO0 & !AC` gate (TRUTH=0x04) needs zero PCB pins;
  full `AC ? WO0 : WO1` mux still needs one loopback pin for WO1.
- Pages updated: [[CCL Configuration]], [[SAMC21 Datasheet Ch.37 CCL]]

## [2026-05-06] query | CCL CTRL naming — CTRLA does not exist

- Build error revealed: CCL register is `CCL->CTRL` (not `CTRLA`) and `CCL_CTRL_ENABLE` (not `CCL_CTRLA_ENABLE`).
- The datasheet chapter 37 and the concept page both used incorrect `CTRLA` naming.
- Fixed in: [[CCL Configuration]] (checklist + init example), `src/ccl_tcc_test.hpp`.

## [2026-05-06] query | CclTccTest experiment written

- Written `src/ccl_tcc_test.hpp` — tests whether CCL INSEL=TCC (0x8) is functional on SAMC21J18A.
- Runs TCC0, TCC1, TCC2 one at a time at ~2 Hz (DIV1024, PER=23436, CC[0]=11718).
- All 4 LUTs configured: INSEL2=TCC(0x8), INSEL1/0=MASK, TRUTH=0x10 (out=IN[2]).
- LUT outputs routed to GPIO function I: LUT0→PB23, LUT1→PA11, LUT2→PA25, LUT3→PB17.
- Sampling 1.5 s per TCC; reports which LUTs toggle → determines TCC→LUT mapping.
- Hooked into `src/main.cpp` after TcTest::run().
- Build: SUCCESS (5208 B flash).

## [2026-05-06] query | INSEL=TCC (0x8) likely functional on SAMC21J18A

- Checked CMSIS header `src/core/include/component/ccl.h`.
- `CCL_LUTCTRL_INSEL0_TCC_Val = 0x8ul` defined as "TCC input source" — NOT reserved.
- Values 0xA (ALT2TC) and 0xB (ASYNCEVENT) are absent — correctly N-series only.
- Datasheet "reserved" claim for 0x8 on C20/C21 is likely another inaccuracy (same pattern as TC2 COUNT32 and TC mapping errata).
- TCC→LUT mapping unknown; needs experimental determination before production use.
- If functional: TCC2 WO signals routable to CCL without external PCB traces.
- Pages updated: [[CCL Configuration]], [[SAMC21 Datasheet Ch.37 CCL]]

## [2026-05-06] query | Silicon revision verified — all errata annulled on Rev F

- DSU.DID = 0x11010500 (read at 0x41002018 via J-Link mem32).
- DEVSEL[7:0] = 0x00 → ATSAMC21J18A confirmed (Table 2 of errata doc: DID=0x1101xx00).
- REVISION[11:8] = 0x5 → **Rev F** (document covers A=0x0 through E=0x4).
- Full analysis: **no errata in DS80000740B has X in the Rev E column for E/G/J devices**.
- Conclusion: all documented errata were fixed by Rev E at the latest → **ALL annulled on Rev F**.
- Key implications for CCL PWM design:
  - GCLK_AC (PCHCTRL[34]) IS functional → use it directly, not GCLK_ADC1
  - CCL TC mapping follows datasheet (TC0/1/2/3 for INSEL=TC on LUT0/1/2/3)
  - EVSYS synchronous channels work without ONDEMAND workaround
  - ADC can use synchronous event path
  - TC I/O pin capture works as documented
  - CCL RS latch reset works as documented
- Pages updated: [[SAMC21 Errata]], [[CCL Configuration]]

## [2026-05-06] ingest | SAMC21 Errata DS80000748 (SAMC20_C21_ERRATA.pdf)

- Read full errata document (40 pages). Target silicon: SAMC21J18A-AU, rev A, J-variant.
- Sources created (1): [[SAMC21 Errata]]
- Key errata affecting this design:
  - **1.7.1 CCL RS Latch**: reset not functional; disable LUT to clear.
  - **1.8.2 AC Clock**: GCLK_AC non-functional; use GCLK_ADC1 (PCHCTRL[36]) instead.
  - **1.8.3 CCL TC Selection**: hardware TC mapping reversed vs datasheet for INSEL=TC and INSEL=ALTTC.
  - **1.12.1 EVSYS**: spurious overrun with always-on GCLK; set ONDEMAND=1.
  - **1.20.2 TC Capture**: I/O pin capture broken; use EVSYS+EIC/CCL path instead.
  - **1.4.4 ADC Event**: use only asynchronous event path.
- Pages created/updated: [[SAMC21 Errata]]

## [2026-05-06] ingest | Datasheet Chapter 37 (CCL)

- Read datasheet pages 849–867 (ch37 CCL — complete chapter).
- Sources created (1): [[SAMC21 Datasheet Ch.37 CCL]]
- Concepts created (1): [[CCL Configuration]]
- Notable: INSEL=TCC (0x8) not available on SAM C20/C21 — reserved.
- Notable: INSEL=AC mapping is fixed: LUT0→COMP0, LUT1→COMP1, LUT2→COMP2, LUT3→COMP3.
- Notable: LINK direction is LUT(n+1).OUT → LUT(n).IN (higher feeds lower).
- Notable: TRUTH=0xCA for `AC?WO0:WO1`; TRUTH=0x40 for `AC?WO0:0`.
- Notable: GCLK_CCL (PCHCTRL[38]) not needed for pure combinational logic.
- Notable: CCL event generators LUTOUT0–3 at EVSYS indices 82–85.
- Pages created/updated: [[SAMC21 Datasheet Ch.37 CCL]], [[CCL Configuration]]

## [2026-05-06] query | Errata datasheet DS60001479D §35.6.2.4 — TC2 e modalità 32-bit

- Il datasheet afferma (p. 704): *"TC2 does not support 32-bit resolution."*
- Esperimento hardware su ATSAMC21J18A-AU (2026-05-06) ha smentito questa affermazione:
  TC2 in COUNT32 mode + TC3 slave → `STATUS.SLAVE=1`, counter = 74971 dopo 1,6 s (> 65535).
- Gli header Atmel (`TC2_MASTER=1`, registri `COUNT32_*` definiti) sono coerenti con il
  funzionamento osservato, non con il datasheet.
- Pages updated: [[TC 32-Bit Paired Mode]], [[SAMC21 Datasheet Ch.35 TC]]

## [2026-05-05] lint | Correzione contenuto non richiesto (tc_test.hpp)

- Rimossa la sezione "Verification Result (tc_test.hpp)" da [[TC 32-Bit Paired Mode]]: affermava "TC0+TC1 and TC2+TC3 confirmed working" come fatto, ma era basata su codice sperimentale non in `raw/` e non ingested su richiesta.
- Rimossi da [[Application Bring-Up Source]]: riga `tc_test.hpp` dalla tabella files, `TcTest::run()` dalla sequenza di boot, nota sui tempi di blocco, link a TC 32-Bit Paired Mode.
- `tc_test.hpp` è in `src/` (fuori da `raw/`); il suo contenuto non costituisce una fonte wiki attestabile.

## [2026-05-05] ingest | Datasheet Chapter 20 (OSCCTRL)

- Read datasheet pages 190–227 (ch20 OSCCTRL — complete chapter, all registers).
- Sources created (1): [[SAMC21 Datasheet Ch.20 OSCCTRL]]
- Concepts updated (1): [[OSCCTRL]] — fixed XOSCCTRL bit-position errors (STARTUP at [15:12], GAIN at [10:8], ONDEMAND at bit 7; previously shown at wrong offsets), corrected GAIN(3)→GAIN(4) in 24 MHz example, updated sources slug.
- Notable: OSC48MDIV is write-synchronized; poll OSC48MSYNCBUSY.OSC48MDIV after writing.
- Notable: DPLLCTRLB is enable-protected: must be written before DPLLCTRLA.ENABLE=1.
- Notable: CAL48M (offset 0x38) exists only on Rev D silicon; load from NVM Software Cal Area.
- Notable: DPLLCTRLB.REFCLK selects reference: 0x0=XOSC32K, 0x1=XOSC, 0x2=GCLK.

## [2026-05-05] ingest | Datasheet Chapter 13 (DSU)

- Read datasheet pages 87–121 (ch13 DSU — complete chapter, all registers).
- Sources created (1): [[SAMC21 Datasheet Ch.13 DSU]]
- Notable: Internal range 0x0000–0x00FF always accessible; external 0x0100–0x01FF blocked when STATUSB.PROT=1.
- Notable: STATUSB.PROT is set at power-up if NVM BOOTPROT is active; never cleared except by Chip Erase.
- Notable: CRC-32 polynomial 0xEDB88320 (reflected); initial value written to DATA before starting.
- Notable: Cold-Plugging detected via STATUSA.CRSTEXT=1; firmware must clear it to release CPU.
- Notable: DID register identifies exact device (PROCESSOR, FAMILY, SERIES, DIE, REVISION, DEVSEL).

## [2026-05-05] ingest | Datasheet Chapter 11 (PAC)

- Read datasheet pages 57–78 (ch11 PAC — complete chapter, all registers).
- Sources created (1): [[SAMC21 Datasheet Ch.11 PAC]]
- Concepts created (1): [[PAC Configuration]]
- Notable: WRCTRL.KEY: CLEAR=0x1, SET=0x2, LOCK=0x3 (permanent until hardware reset).
- Notable: PERID = 32×BridgeNumber+N where N from Ch.12 "PAC, Index" column.
- Notable: INTFLAGAHB/A/B/C/D cover all AHB slaves and APB-A/B/C/D peripherals with per-peripheral bits.
- Notable: PAC state not reset by user SWRST — only hardware reset clears write-protection.
- Pages created/updated: [[SAMC21 Datasheet Ch.11 PAC]], [[PAC Configuration]]

## [2026-05-05] ingest | Datasheet Chapter 44 (FREQM)

- Read datasheet pages 1017–1030 (ch44 FREQM — complete chapter with all registers).
- Sources created (1): [[SAMC21 Datasheet Ch.44 FREQM]]
- Concepts created (1): [[FREQM Configuration]]
- Notable: Formula f_CLK_MSR = (VALUE/REFNUM) × f_CLK_REF. Reference must be slower.
- Notable: GCLK PCHCTRL[3]=MSR (measured), PCHCTRL[4]=REF (reference).
- Notable: CFGA.DIVREF (÷8 reference prescaler) only on N-series variants.
- Notable: STATUS.OVF is sticky — cleared by writing 1 separately from new measurement.

## [2026-05-05] ingest | Datasheet Chapter 15 (Clock System)

- Read datasheet pages 133–136 (ch15 Clock System overview).
- Sources created (1): [[SAMC21 Datasheet Ch.15 Clock System]]
- Concepts updated (0): info already captured in [[Clock System]] concept.
- Notable: 5-step peripheral enable sequence (clock source → GCLK gen → PCHCTRL → APB mask → peripheral enable).
- Notable: Sync delay formula: 5·PGCLK + 2·PAPB < D < 6·PGCLK + 3·PAPB.
- Notable: WRTLOCK generic clocks survive user reset; 32kHz sources only reset by power-on.
- Notable: On-demand mode: clock stops when no peripheral requesting (save power).

## [2026-05-05] ingest | Datasheet Chapter 12 (Peripherals Configuration Summary)

- Read datasheet pages 79–86 (ch12 — 4 variant tables: SAM C21 N, C20 N, C21 E/G/J, C20 E/G/J).
- Sources created (1): [[SAMC21 Datasheet Ch.12 Peripherals Configuration Summary]]
- Concepts updated (0): base addresses already in [[Memory Map]]; GCLK indexes now authoritative.
- Notable: APB-A clocks ALL enabled at reset; APB-C/D clocks NONE enabled at reset.
- Notable: Shared GCLK slots: TC0/1 share PCHCTRL[30]; TC2/3 share [31]; TCC0/1 share [28]; SERCOMs share SLOW [18] or [24].
- Notable: SERCOM5 PCHCTRL: CORE=25, SLOW=24. AC GCLK index differs between N (40) and E/G/J (34-area).
- Notable: TSENS APB not enabled at reset despite being on APB-A bridge — requires explicit MCLK enable.

## [2026-05-05] ingest | Datasheet Chapter 10 (Processor and Architecture)

- Read datasheet pages 49–54 (ch10 — Cortex-M0+ config, NVIC tables, MTB, bus system).
- Sources created (1): [[SAMC21 Datasheet Ch.10 Processor and Architecture]]
- Concepts updated (1): [[NVIC Interrupt Map]] — fixed sources slug, added SAM C20 table.
- Notable: SAM C21 line 0 = PM+MCLK+OSCCTRL+OSC32KCTRL+SUPC+PAC (all shared).
- Notable: Wake-up Interrupt Controller (WIC) NOT present on SAM C20/C21.
- Notable: 8-region MPU present; VTOR present; "Reset all registers" absent.
- Notable: Single-cycle I/O port bus for PORT/DIVAS separate from AHB-Lite.

## [2026-05-05] ingest | Datasheet Chapter 9 (Memories)

- Read datasheet pages 45–48 (ch9 — physical memory map, NVM special areas, serial number).
- Sources created (1): [[SAMC21 Datasheet Ch.9 Memories]]
- Concepts updated (1): [[Memory Map]] — fixed sources slug (was samc21-datasheet-ch09, now correct).
- Notable: NVM User Row 0x804000; Software Calibration 0x806020; Temp Calibration 0x806030.
- Notable: Temperature Calibration Area (TSENS) present only on SAM C21, not SAM C20.
- Notable: Serial number: 4 words at 0x0080A00C, 040, 044, 048 — all 128 bits needed for uniqueness.
- Notable: CAL48M has separate values for VDD 3.6–5.5V and 2.7–3.6V ranges.

## [2026-05-05] ingest | Datasheet Chapter 5 (Signal Descriptions)

- Read datasheet pages 25–26 (ch5 Signal Descriptions — complete peripheral signal list).
- Sources created (1): [[SAMC21 Datasheet Ch.5 Signal Descriptions]]
- Concepts updated (0): signal data merged into existing [[I/O Multiplexing]] context.
- Notable: All SERCOM expose PAD[3:0]; function determines role.
- Notable: ADC 20 channels AIN[19:0]; AC 8 inputs AIN[7:0]; PTC 16×16 X/Y matrix.
- Notable: CAN TX/RX, DAC VOUT, SDADC differential pairs AINP/AINN[2:0] listed.

## [2026-05-05] ingest | Datasheet Chapter 4 (Pinout)

- Read datasheet pages 20–24 (ch4 Pinout — all four package variants).
- Sources created (1): [[SAMC21 Datasheet Ch.4 Pinout]]
- Concepts created (1): [[Package Variants]]
- Notable: Four variants — E(32), G(48), J(64+WLCSP56), N(100 with Port C).
- Notable: Project target is ATSAMC21J18A-AU (64-pin VQFN, full PA+PB).
- Notable: SERCOM5 PB30/PB31 UART requires J or N variant (PB absent on E/G partial).

## [2026-05-05] ingest | Datasheet Chapter 43 (TSENS)

- Read datasheet pages 994–1013 (ch43 TSENS — complete chapter: overview, functional description, all register descriptions up to GAIN).
- Sources created (1): [[SAMC21 Datasheet Ch.43 TSENS]]
- Concepts created (1): [[TSENS Configuration]]
- Notable: Present on both SAM C20 and SAM C21. Time-domain temperature measurement.
- Notable: Result: 24-bit signed two's complement; 100 counts = 1°C (GCLK=48MHz, NVM calibration loaded).
- Notable: 25°C → VALUE=2500=0x09C4; −25°C → VALUE=−2500=0xFFF63C.
- Notable: Calibration mandatory — GAIN, OFFSET, FCAL, TCAL from NVM Temperature Calibration Area.
- Notable: GAIN and OFFSET NOT reset by CTRLA.SWRST; persist across software resets.
- Notable: GAIN factory-calibrated for OSC48M; scale GAIN if different GCLK_TSENS frequency used.
- Notable: Enable-protected: CTRLA.RUNSTDBY, CTRLC, EVCTRL, WINLT, WINUT, GAIN, OFFSET, CAL.
- Notable: Write-synchronized: CTRLA.SWRST and CTRLA.ENABLE only.
- Notable: INTFLAG.RESRDY/WINMON cleared by writing 1 OR reading VALUE.
- Notable: STATUS.OVF=1 means result invalid (>24 bits required).
- Notable: Datasheet recommends averaging 10 measurements for accuracy.
- Notable: Window monitor: 7 modes including hysteresis (HYST_ABOVE/HYST_BELOW).
- All planned datasheet chapters now ingested.

## [2026-05-05] ingest | Datasheet Chapter 41 (DAC)

- Read datasheet pages 972–989 (ch41 DAC — complete chapter: overview, functional description, all register descriptions).
- Sources created (1): [[SAMC21 Datasheet Ch.41 DAC]]
- Concepts created (1): [[DAC Configuration]]
- Notable: SAM C21 only. One channel, 10-bit, up to 350 ksps.
- Notable: Enable-protected: CTRLB, EVCTRL.
- Notable: Write-synchronized: CTRLA.SWRST/ENABLE, DATA, DATABUF — bus error if written while SYNCBUSY set.
- Notable: Two-stage FIFO: write DATABUF, START event loads into DATA and starts conversion.
- Notable: CTRLB.EOEN=1 for VOUT pin; CTRLB.IOEN=1 for internal AC/ADC/SDADC use.
- Notable: Dithering mode (CTRLB.DITHER=1): 14-bit effective resolution via 16 sub-conversions.
- Notable: VPD=1 disables voltage pump — only safe above 2.5V VDDANA.
- Notable: STATUS.READY: wait for this after enabling before writing DATA.
- Notable: CTRLA.RUNSTDBY=1: output buffer holds last value in STANDBY.
- Next priority: TSENS (Ch.43, pp.994-1016).

## [2026-05-05] ingest | Datasheet Chapter 40 (AC)

- Read datasheet pages 945–964 (ch40 AC — overview, functional description, register summary, CTRLA through STATUSB).
- Sources created (1): [[SAMC21 Datasheet Ch.40 AC]]
- Concepts created (1): [[AC Configuration]]
- Notable: Four comparators (COMP0–3) in two pairs; COMP0/COMP1 = pair 0, COMP2/COMP3 = pair 1.
- Notable: Only EVCTRL is enable-protected; COMPCTRLn and most other registers are NOT.
- Notable: Write-synchronized: CTRLA.SWRST/ENABLE, COMPCTRLn.ENABLE, WINCTRL.
- Notable: Continuous vs single-shot (SINGLE bit); hysteresis and filter only in continuous mode.
- Notable: Window mode (WENx): both comparators in pair must share same positive input.
- Notable: VDD scaler: 64 levels, V_out = V_DD × (VALUE+1)/64; used when MUXNEG=5 or MUXPOS=4.
- Notable: Continuous mode with RUNSTDBY: asynchronous detection in STANDBY without GCLK_AC.
- Notable: STATUSA.WSTATE: ABOVE=0x0, INSIDE=0x1, BELOW=0x2.
- Next priority: DAC (Ch.41, pp.972-989), TSENS (Ch.43, pp.994-1016).

## [2026-05-05] ingest | Datasheet Chapter 39 (SDADC)

- Read datasheet pages 910–929 (ch39 SDADC — overview, functional description, register summary, CTRLA through SEQSTATUS).
- Sources created (1): [[SAMC21 Datasheet Ch.39 SDADC]]
- Concepts created (1): [[SDADC Configuration]]
- Notable: SAM C21 only (not SAM C20). One instance.
- Notable: 16-bit signed differential result; 3 channel pairs (AINP0/AINN0, AINP1/AINN1, AINP2/AINN2).
- Notable: Clock chain: f_CLK_SDADC = f_GCLK/(2×(PRESCALER+1)); f_output = f_CLK/(4×OSR).
- Notable: OSR must not be changed while running — reset SDADC first.
- Notable: First valid sample always from 3rd sample onward (settling); SKPCNT adds extra skip.
- Notable: Enable-protected: CTRLA.ONDEMAND/RUNSTDBY, CTRLB, CTRLC, EVCTRL, ANACTRL.
- Notable: REFCTRL is enable-protected; ONREFBUF=1 required for INTREF or DAC reference.
- Notable: Correction formula: (Data0 + OFFSETCORR) × GAINCORR / 2^SHIFTCORR, saturated to 16 bits.
- Notable: Chopper mode (ANACTRL.ONCHOP=1) reduces offset error.
- Notable: Write-synchronized registers → bus error if written while SYNCBUSY set.
- Next priority: AC (Ch.40, pp.945-971), DAC (Ch.41, pp.972-989), TSENS (Ch.43, pp.994-1016).

## [2026-05-05] ingest | Datasheet Chapter 38 (ADC)

- Read datasheet pages 868–887 (ch38 ADC — overview, functional description, register summary, CTRLA/CTRLB/REFCTRL).
- Sources created (1): [[SAMC21 Datasheet Ch.38 ADC]]
- Concepts created (1): [[ADC Configuration]]
- Notable: Two instances ADC0 and ADC1; can run as master/slave for simultaneous sampling.
- Notable: Minimum prescaler is DIV2; for 1 MSPS at 12-bit need GCLK_ADC ≥ 32 MHz.
- Notable: Enable-protected: CTRLB, REFCTRL, EVCTRL, CALIB — configure before ENABLE=1.
- Notable: Double-buffered: INPUTCTRL, CTRLC, AVGCTRL, SAMPCTRL, WINLT, WINUT, GAINCORR, OFFSETCORR.
- Notable: CALIB (BIASCOMP, BIASREFBUF) must be loaded from NVM at 0x00800080 before enabling.
- Notable: Rail-to-rail mode (CTRLC.R2R=1) requires OFFCOMP=1 (fixes sampling at 4 cycles).
- Notable: Oversampling to 16-bit: SAMPLENUM=0x8 (256 samples), ADJRES=0 → 4 automatic right-shifts.
- Notable: OVERRUN interrupt is NOT a sleep wakeup source.
- Notable: ADC uses asynchronous event channels only; EVCTRL.STARTINV for falling edge trigger.
- Next priority: SDADC (Ch.39), AC (Ch.40), DAC (Ch.41), TSENS (Ch.43).

## [2026-05-05] ingest | Datasheet Chapter 36 (TCC)

- Read datasheet pages 774–813 (ch36 TCC — architecture, waveform modes, fault handling, register summary, CTRLA description).
- Sources created (1): [[SAMC21 Datasheet Ch.36 TCC]]
- Concepts created (1): [[TCC Configuration]]
- Notable: TCC0/TCC1 share GCLK_TCC peripheral channel; TCC2 is independent.
- Notable: Counter widths: TCC0/1 = 24-bit, TCC2 = 16-bit.
- Notable: WAVE, PER, CCx, PATT are write-synchronized — always poll SYNCBUSY after writes.
- Notable: COUNT read requires explicit READSYNC command (CTRLBSET.CMD=READSYNC).
- Notable: Double-buffered: CCBUFx/PERBUF/WAVEBUF/PATTBUF → applied at UPDATE condition.
- Notable: CTRLB.LUPD=1 suspends buffer updates; CTRLA.ALOCK auto-sets LUPD on overflow.
- Notable: Fault blanking window suppresses re-arm glitches; FILTERVAL filters async inputs.
- Notable: Output matrix OTMX configures WO[0:7] routing; DTI inserts dead time per channel pair.
- Notable: BLDC pattern generation via PATT/PATTBUF (PGE enable + PGV value per WO pin).
- Next priority: ADC (Ch.38, pp.868-909), SDADC, AC, DAC, TSENS.

## [2026-05-05] ingest | Datasheet Chapter 34 (CAN)

- Read datasheet pages 605–644 (ch34 CAN — overview, architecture, register summary, key registers).
- Sources created (1): [[SAMC21 Datasheet Ch.34 CAN]]
- Concepts created (1): [[CAN Configuration]]
- Notable: SAM C21 only (not SAM C20). ISO 11898-1:2015 / CAN 2.0 A/B / CAN-FD (64 bytes, 10 Mbps).
- Notable: Message RAM is in system SRAM, accessed via AHB — must allocate before enabling.
- Notable: Addresses in configuration registers are word addresses (byte_addr >> 2).
- Notable: Configuration write access requires CCCR.INIT=1 AND CCCR.CCE=1.
- Notable: TXBAR/TXBCR writable only while CCCR.CCE=0 (normal operation).
- Notable: CCCR.INIT write-synchronized — read back INIT before setting new value.
- Notable: Sleep: CCCR.CSR=1 → wait CCCR.CSA=1 → stop clocks. Wake: restart clocks → CSR=0.
- Notable: FIFO Acknowledge: write get index to RXF0A/RXF1A/TXEFA to advance get index.
- Notable: CAN FD bit rate switching: CCCR.FDOE+BRSE=1 + Tx buffer FDF+BRS=1.
- Notable: Bus-Off auto-sets CCCR.INIT; application must clear INIT after recovery.
- Notable: ECR.CEL — error logging counter; read to clear; caps at 0xFF → sets IR.ELO.
- Next priority: TCC (Ch.36, pp.774-849), ADC (Ch.38, pp.868-909), SDADC, AC, DAC, TSENS.

## [2026-05-05] ingest | Datasheet Chapter 33 (SERCOM I2C)

- Read datasheet pages 555–604 (ch33 SERCOM I2C — both slave and master register descriptions).
- Sources created (1): [[SAMC21 Datasheet Ch.33 SERCOM I2C]]
- Concepts created (1): [[SERCOM I2C Configuration]]
- Notable: PAD[0]=SDA, PAD[1]=SCL — fixed, unlike SPI DOPO/DIPO.
- Notable: Two separate GCLK inputs: GCLK_SERCOMx_CORE (required) + GCLK_SERCOM_SLOW (SMBus timeouts only).
- Notable: Master SYNCBUSY.SYSOP: poll after CTRLB.CMD, STATUS.BUSSTATE, ADDR, or DATA writes.
- Notable: BUSSTATE=UNKNOWN after reset; must write 0x1 to force IDLE before first transaction.
- Notable: ADDR bit 0 is R/W flag (0=write, 1=read); 7-bit address in bits 7:1.
- Notable: Writing ADDR triggers the bus operation and clears STATUS.BUSERR/ARBLOST/INTFLAG.MB/SB.
- Notable: ACKACT=1 (NACK) must be set before reading the last byte in a master read to terminate.
- Notable: DMA: ADDR.LENEN=1 + ADDR.LEN=N → hardware auto-NACKs and issues STOP after N bytes.
- Notable: Slave SYNCBUSY has only ENABLE/SWRST (no SYSOP) — slave DATA is write+read synchronized.
- Notable: LENERR (STATUS.10) set when slave NACKs before all LEN bytes transferred in DMA mode.
- Next priority: TCC (pp.699–773), ADC (pp.868–909), SDADC, AC, DAC.

## [2026-05-05] ingest | Datasheet Chapter 32 (SERCOM SPI)

- Read datasheet pages 528–554 (ch32 SERCOM SPI).
- Sources created (1): [[SAMC21 Datasheet Ch.32 SERCOM SPI]]
- Concepts created (1): [[SERCOM SPI Configuration]]
- Notable: CTRLA (except ENABLE/SWRST), CTRLB (except RXEN), BAUD, ADDR are enable-protected.
- Notable: Three SYNCBUSY points: SWRST, ENABLE, and CTRLB.RXEN — must poll after each.
- Notable: Writing CTRLB while SYNCBUSY.CTRLB=1 causes APB bus error.
- Notable: f_SCK = GCLK / (2*(BAUD+1)); BAUD=5 → 4 MHz at 48 MHz GCLK.
- Notable: DOPO selects DO+SCK+SS pad bundle; DIPO selects DI pad independently.
- Notable: DRE/RXC flags are read-only; cleared only by writing/reading DATA respectively.
- Notable: TXC required before deasserting SS to ensure last byte fully shifted out.
- Notable: Slave first character comes from shift register content — use PLOADEN=1 to preload.
- Notable: IBON=1 raises BUFOVF immediately on overflow; IBON=0 (default) propagates through FIFO.
- Next priority: SERCOM I2C (pp.555–604), CAN (pp.605–698), TCC (pp.774–849), ADC (pp.868–909).

## [2026-05-05] ingest | Datasheet Chapter 29 (EVSYS)

- Read datasheet pages 463–483 (ch29 EVSYS).
- Sources created (1): [[SAMC21 Datasheet Ch.29 EVSYS]]
- Concepts created (1): [[EVSYS]]
- Notable: 12 channels, 95 generators, 47 users. EVSYS is always enabled — only CTRLA.SWRST to reset.
- Notable: USERm.CHANNEL = channel_number + 1 (1-based, 0=disconnected). Always configure USER before CHANNELn.
- Notable: CHANNELn must be written as a single 32-bit word.
- Notable: Asynchronous PATH (0x2): no GCLK, no interrupts, EDGSEL must be 0; user peripheral handles edge detection.
- Notable: CHSTATUS.USRRDYn resets to 0xFF — do not use as a "channel connected" indicator.
- Notable: CHANNELn resets to 0x00008000 (ONDEMAND=1); RUNSTDBY=1 needed for standby operation.
- Notable: SWEVT write-only strobe — write 1 to bit n triggers software event on channel n.
- Next priority: SERCOM SPI (pp.528–554), SERCOM I2C (pp.555–604), CAN (pp.605–698), TCC (pp.774–849), ADC (pp.868–909), SDADC (pp.910–944), AC (pp.945–971), DAC (pp.972–989), TSENS (pp.994–1016).

## [2026-05-05] ingest | Datasheet Chapters 24 (RTC) + 26 (EIC) + 27 (NVMCTRL)

- Read datasheet pages 279–327 (ch24 RTC), 385–409 (ch26 EIC), 410–434 (ch27 NVMCTRL).
- Sources created (3): [[SAMC21 Datasheet Ch.24 RTC]], [[SAMC21 Datasheet Ch.26 EIC]], [[SAMC21 Datasheet Ch.27 NVMCTRL]]
- Concepts created (3): [[RTC]], [[EIC]], [[NVMCTRL]]
- Notable RTC: Three modes — COUNT32, COUNT16, CLOCK/Calendar. APBA bus (not APBB). Persists through WDT/EXT/SYST resets; cleared only by POR or SWRST.
- Notable RTC: For accurate 1 Hz calendar use XOSC1K (1024 Hz, RTCSEL=0x4) + PRESCALER=DIV1024. OSCULP32K-derived clocks are ~1% accurate only.
- Notable RTC: CLOCK register (Mode 2) is read-synchronized; set CTRLA.CLOCKSYNC=1 and wait SYNCBUSY.CLOCK before reading. All write-synchronized registers require SYNCBUSY polling after every write.
- Notable RTC: YEAR[1:0]=0 is treated as a leap year. DBGRUN is NOT cleared by SWRST.
- Notable RTC: PERn interrupt fires at CLK_RTC / 2^(n+2); with 1024 Hz source: PER7 = 2 Hz, PER0 = 256 Hz.
- Notable RTC: MASK.SEL table for Mode 2 alarm: 0x0=OFF, 0x1=SS, 0x2=MMSS, 0x3=HHMMSS, 0x4=DDHHMMSS, 0x5=MMDDHHMMSS, 0x6=YYMMDDHHMMSS.
- Notable EIC: SAMC21J18A is NOT an "N" variant — DEBOUNCEN/DPRESCALER/PINSTATE registers absent.
- Notable EIC: APBB bus. GCLK_EIC not needed for level detection or async edge detection; required only for sync edge / filtering.
- Notable EIC: ASYNCH[x]=1 enables wakeup from STANDBY with all clocks stopped. FILTENx and ASYNCH[x] are mutually exclusive.
- Notable EIC: Enable-protected registers (EVCTRL, CONFIGn, ASYNCH, CTRLA.CKSEL) must be written before ENABLE=1.
- Notable NVMCTRL: Wait states MUST be set (CTRLB.RWS=1) BEFORE switching CPU above 24 MHz — failure causes flash read errors.
- Notable NVMCTRL: CTRLB.MANW resets to 1 — must issue WP command to commit page buffer.
- Notable NVMCTRL: ADDR register uses 16-bit word addressing: ADDR = byte_addr >> 1.
- Notable NVMCTRL: Always erase entire row (4 pages × 64 bytes) before writing any page in it.
- Notable NVMCTRL: SSB (Set Security Bit, cmd 0x45) is irreversible without debugger chip-erase.
- Next priority: EVSYS (pp.463–483), SERCOM SPI (pp.528–554), SERCOM I2C (pp.555–604), CAN (pp.605–698), TCC (pp.774–849), ADC (pp.868–909), SDADC (pp.910–944), AC (pp.945–971), DAC (pp.972–989), TSENS (pp.994–1016).

## [2026-05-05] ingest | Datasheet Chapters 18 (RSTC) + 19 (PM) + 22 (SUPC)

- Read datasheet pages 176–189 (ch18 RSTC, ch19 PM) and 249–263 (ch22 SUPC).
- Sources created (3): [[SAMC21 Datasheet Ch.18 RSTC]], [[SAMC21 Datasheet Ch.19 PM]], [[SAMC21 Datasheet Ch.22 SUPC]]
- Concepts created (3): [[RSTC]], [[PM]], [[SUPC]]
- Notable: RSTC.RCAUSE: power-supply resets (POR/BODCORE/BODVDD) reset only RTC/OSC32KCTRL/RSTC/GCLK-WRTLOCK; user resets (EXT/WDT/SYST) reset everything.
- Notable: PM.SLEEPCFG: valid values are IDLE=0x2 and STANDBY=0x4 only; must read-back verify before WFI.
- Notable: SUPC.VREG.ENABLE must always remain 1 — clearing it disables the internal regulator.
- Notable: SUPC.VREF SEL=0x1 is reserved; use 0x0 (1.024V), 0x2 (2.048V), 0x3 (4.096V).
- Notable: SUPC.BODVDD is enable-protected; wait STATUS.BVDDSRDY=1 after enabling before trusting BODVDDDET.
- Notable: BODCORE is factory-calibrated, non-configurable, always disabled in standby.
- Next priority: EIC (pp. 385-409), NVMCTRL (pp. 410-434), RTC (pp. 279-327), EVSYS (pp. 463-483).

## [2026-05-05] ingest | Datasheet Chapters 35 (TC) + 23 (WDT)

- Read datasheet pages 699–773 (ch35 TC) and 264–278 (ch23 WDT).
- Sources created (2): [[SAMC21 Datasheet Ch.35 TC]], [[SAMC21 Datasheet Ch.23 WDT]]
- Concepts created (1): [[WDT]] — init sequence, timeout/window periods, CLEAR=0xA5 pattern, always-on mode
- Concepts updated (1): [[TC 32-Bit Paired Mode]] — expanded to cover all modes (8/16/32-bit), added full register reference tables, corrected pairing (TC0+TC1/TC2+TC3/TC4+TC5), added waveform modes, EVACT, CTRLB CMD table
- Notable: CTRLBSET.CMD=READSYNC (0x4) required before every COUNT read; wait SYNCBUSY.COUNT.
- Notable: CTRLA fields (except ENABLE/SWRST) are enable-protected; configure while ENABLE=0.
- Notable: 32-bit mode pairs are even+odd (TC0/TC2/TC4 = master); slave TC needs APB clock but not GCLK.
- Notable: STATUS.STOP resets to 1 — TC starts stopped; RETRIGGER or ENABLE starts counting.
- Notable: PERBUFV in STATUS is 8-bit mode only; always reads 0 in 16/32-bit mode.
- Notable: WDT clocked from OSCULP32K (~1 kHz) — timeout periods are approximate, not precise.
- Notable: WDT ALWAYSON cannot be cleared by software — only POR resets it.
- Notable: WDT CLEAR: write 0xA5 to pet; any other value causes immediate reset; must check SYNCBUSY.CLEAR before each write.
- Notable: NVM User Row sets default values for WDT CTRLA/CONFIG/EWCTRL on POR.
- Next priority: EIC (pp. 385-409), NVMCTRL (pp. 410-434), RSTC+PM (pp. 176-189), SUPC (pp. 249-263).

## [2026-05-05] ingest | Datasheet Chapter 25 (DMAC)

- Read datasheet pages 328–384 (ch25 DMAC).
- Sources created (1): [[SAMC21 Datasheet Ch.25 DMAC]]
- Concepts updated (1): [[DMA Controller]] — added base address, global/per-channel register tables, BTCTRL bit field table, TRIGACT/TRIGSRC tables, updated source in frontmatter
- Notable: CHID must be written before every CH* register access — not atomic, needs IRQ critical section if used from ISR and main.
- Notable: BASEADDR and WRBADDR must be 16-byte aligned; memory section size = 16 × (highest_channel + 1).
- Notable: BTCTRL.VALID must be 1; DMAC suspends channel on VALID=0 descriptor (instead of bus error).
- Notable: TRIGACT=BEAT is the correct mode for peripheral streaming (one trigger per byte from SERCOM DRE/RXC).
- Notable: SERCOM5 TX = 0x0D, SERCOM5 RX = 0x0C — confirmed matches CMSIS macros SERCOM5_DMAC_ID_TX/RX.
- Notable: CTRL.SWRST requires both DMAENABLE=0 and CRCENABLE=0 before writing.
- Notable: DESCADDR=0 terminates transaction; DESCADDR=self creates circular transfer.
- Next priority: TC chapter (pp. 699–773).

## [2026-05-05] ingest | Datasheet Chapter 31 (SERCOM USART)

- Read datasheet pages 492–527 (ch31 SERCOM USART).
- Sources created (1): [[SAMC21 Datasheet Ch.31 SERCOM USART]]
- Concepts updated (1): [[SERCOM UART Configuration]] — added register-level CTRLA/CTRLB/STATUS/SYNCBUSY tables, updated TXPO/RXPO with full datasheet tables, added datasheet source to frontmatter
- Notable: CTRLA and CTRLB are enable-protected; must write while ENABLE=0 (after SWRST).
- Notable: SYNCBUSY must be polled after SWRST, CTRLB write, and ENABLE write — three distinct sync points.
- Notable: DRE triggers DMA when TX buffer empty; RXC triggers DMA on receive complete.
- Notable: TXPO=0x2 activates RTS/CTS on PAD[2]/PAD[3]; TXPO=0x1 moves TX to PAD[2] — board uses TXPO=0 (TX=PAD[0]).
- Notable: BAUD arithmetic mode: `BAUD = 65536 × (1 − 16 × f_baud / f_ref)`; fractional splits into 13-bit integer + 3-bit FP.
- Next priority: DMAC chapter (pp. 328–384), then TC (pp. 699–773).

## [2026-05-05] ingest | Datasheet Chapters 6 (I/O Mux) + 28 (PORT)

- Read datasheet pages 20–35 (ch4 Pinout, ch5 Signal List, ch6 I/O Multiplexing) and 435–462 (ch28 PORT).
- Sources created (2): [[SAMC21 Datasheet Ch.6 I/O Multiplexing]], [[SAMC21 Datasheet Ch.28 PORT]]
- Concepts created (2): [[PORT]], [[I/O Multiplexing]]
- Notable: Function B must be selected for all analog pins to disable digital buffer (avoid leakage).
- Notable: SERCOM5 function D: PB30=PAD[0] (TX), PB31=PAD[1] (RX) — confirmed for this board.
- Notable: XOSC/XOSC32K pins (PA14/PA15, PA00/PA01) are not PORT-muxed — leave PINCFG=0x00.
- Notable: SWD on PA30/PA31 (VDDIN domain); auto-routed by debugger.
- Notable: PORT reset state = all inputs with buffers disabled — correct for analog pins without any config write.
- Notable: WRCONFIG is write-only and 32-bit only; 8/16-bit writes have no effect.
- Next priority: SERCOM USART chapter (pp. 492–527), then DMAC (pp. 328–384), then TC (pp. 699–773).

## [2026-05-05] ingest | Datasheet Chapters 20 detail + Chapter 21 OSC32KCTRL

- Read datasheet pages 210–248 (ch20 OSCCTRL register detail, ch21 OSC32KCTRL complete).
- Sources created (1): [[SAMC21 Datasheet Ch.21 OSC32KCTRL]]
- Concepts created (1): [[OSC32KCTRL]]
- Concept updated (1): [[OSCCTRL]] — added XOSCCTRL STARTUP table (31µs–1000ms, 16 entries), GAIN table (0x0=2MHz…0x4=30MHz), OSC48MDIV full 16-entry frequency table (48MHz–3MHz), DPLLCTRLB LTIME and FILTER tables, DIV formula for XOSC reference
- Notable: XOSC32K STARTUP times are 0.06–8 s; must use generous timeout in firmware.
- Notable: OSC32K CALIB[6:0] must be loaded from NVM 0x806020 bits 18:12 — not auto-applied.
- Notable: OSCULP32K is always-on after POR, auto-calibrated — no firmware init required.
- Notable: OSC32KCTRL CFD safe clock is OSCULP32K (not OSC48M like OSCCTRL's CFD).
- Notable: XOSC32K EN32K must be set to use crystal as GCLK source; EN1K is only for RTC 1kHz path.
- Next priority: PORT chapter (pp.435–462) for I/O mux tables, then SERCOM USART (pp.492–527).

## [2026-05-05] ingest | Datasheet Chapters 9, 10, 16, 17, 20

- Read datasheet pages 45–64 (ch9 Memories, ch10 Processor/NVIC) and 133–173 (ch15–17 Clock System, GCLK, MCLK) and 190–209 (ch20 OSCCTRL).
- Sources created (2): [[SAMC21 Datasheet Ch.16 GCLK]], [[SAMC21 Datasheet Ch.17 MCLK]]
- Concepts created (3): [[Memory Map]], [[NVIC Interrupt Map]], [[OSCCTRL]]
- Concept updated (1): [[Clock System]] — fixed reset default (÷12 = 4 MHz, not ÷2), added datasheet cross-refs
- Notable: NVIC line 0 is shared by PM/MCLK/OSCCTRL/OSC32KCTRL/SUPC/PAC — ISR must check all INTFLAG registers.
- Notable: APBCMASK reset=0x00000000 — all SERCOM/TC/ADC clocks disabled by default, must enable before access.
- Notable: NVM Software Calibration at 0x806020 must be read and applied to OSCCTRL.CAL48M at startup; two variants for VDD ≥3.6V vs ≤3.6V.
- Notable: OSC48M starts at ÷12 = 4 MHz; firmware init.hpp must write OSC48MDIV=0 to reach 48 MHz.
- Next priority: read OSCCTRL register details pp.210–227 (XOSCCTRL, full DPLL registers), then OSC32KCTRL (pp.228–248), then PORT chapter (pp.435–462) for I/O mux tables.

## [2026-05-05] ingest | Firmware Core Library + Application Bring-Up

- Ingested all files in `src/core/` and `src/` application layer.
- Sources: `firmware-core-library`, `application-bring-up`
- Entities created (11): [[Pin]], [[RingBuffer]], [[ByteStream and Print Mixin]], [[SerialPort]],
  [[Uart INT]], [[Uart DMA]], [[Timebase]], [[LineReader]], [[Startup SAMC21]], [[Syscalls]], [[QuartzTest]]
- Concepts created (5): [[Clock System]], [[SERCOM UART Configuration]], [[DMA Controller]],
  [[IRQ Critical Section]], [[TC 32-Bit Paired Mode]]
- Notable: `uart_dma.hpp` has a `snapshot_rx_position()` function that suspends the DMA channel
  to read BTCNT atomically — documented in [[DMA Controller]] and [[Uart DMA]].
- Notable: DMAC descriptor SRCADDR/DSTADDR points to **end+1** of the buffer when increment is
  enabled — documented as a gotcha in [[DMA Controller]].
- Next priority: ingest SAMC20_C21 datasheet section by section, starting with OSCCTRL and GCLK.

## [2026-05-05] schema-update | Initial wiki setup

- Created CLAUDE.md agent schema with ingest, query, lint, and schema-update workflows.
- Established directory structure: raw/, wiki/sources/, wiki/entities/, wiki/concepts/, wiki/analyses/.
- Created index.md and log.md.
- Wiki is empty; ready for first ingest.
