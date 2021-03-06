/*
 * File:		dac12bit.c
 * Purpose:		function implementations
 * Adapted from demo code provided by NXP for the K70.
 */

#include "dac12bit.h"
#include "univio.h"
#include "devices.h"
#include "svc.h"

byte DAC1_UP_PTR =0;

byte watermarkIndicator[16][16];
int DACC2_DACBFUP= 0 ;
int DACC2_DACBFRP = 0 ;


int bottom_isr = 0 ;
int top_isr = 0 ;
int watermark_isr = 0 ;

struct waveType waveIndex[] = {
    {"sawtooth", SAWTOOTH},
    {"square", SQUARE}
};

int curWaveIndex = 0;

/********************************************************************/
void dac0_12bit_isr (void) {

    if ((DAC_SR_REG(DAC0_BASE_PTR)&DAC_SR_DACBFRPBF_MASK) ) {
        //Buffer read pointer is equal to top limit
        bottom_isr++;
        DAC_SR_REG(DAC0_BASE_PTR)=Clear_DACBFRPBF;
    }
    else if ((DAC_SR_REG(DAC0_BASE_PTR)&DAC_SR_DACBFRPTF_MASK) ) {
        //buffer read pointer is zero
        top_isr++;
        DAC_SR_REG(DAC0_BASE_PTR)=Clear_DACBFRPTF ;
    }
    else if ((DAC_SR_REG(DAC0_BASE_PTR)&DAC_SR_DACBFWMF_MASK)) {
        //buffer read pointer reaches the watermark level
        watermark_isr++;
        DAC_SR_REG(DAC0_BASE_PTR)= Clear_DACBFWMF ;
    }

    return;
}


void dac1_12bit_isr (void) {


    //Record current read pointer position and up limit position.
    DACC2_DACBFUP =  DAC_C2_DACBFUP_MASK & DAC_C2_REG(DAC1_BASE_PTR);
    DACC2_DACBFRP =  (DAC_C2_DACBFRP_MASK & DAC_C2_REG(DAC1_BASE_PTR))>>4;

    //Record the watermark flag at each read pointer position: Watermark conditions marked as 0x04, else 0x00.
    watermarkIndicator[DACC2_DACBFUP][DACC2_DACBFRP]= DAC_SR_REG(DAC1_BASE_PTR)& DAC_SR_DACBFWMF_MASK  ;

    if ((DAC_SR_REG(DAC1_BASE_PTR)&DAC_SR_DACBFRPBF_MASK) ) {
        //Buffer read pointer is equal to top limit
        bottom_isr++;

        DAC1_UP_PTR++;
        if (DAC1_UP_PTR !=16) {
            //set up limit base on i counter
            DAC_C2_REG(DAC1_BASE_PTR)= DAC_SET_PTR_AT_BF(0) | DAC_SET_PTR_UP_LIMIT(DAC1_UP_PTR);
        }
        DAC_SR_REG(DAC1_BASE_PTR)=Clear_DACBFRPBF;

    }
    else if ((DAC_SR_REG(DAC1_BASE_PTR)&DAC_SR_DACBFRPTF_MASK) ) {
        //buffer read pointer is zero
        top_isr++;
        DAC_SR_REG(DAC1_BASE_PTR)=Clear_DACBFRPTF ;

    }
    else if ((DAC_SR_REG(DAC1_BASE_PTR)&DAC_SR_DACBFWMF_MASK)) {
        //buffer read pointer reaches the watermark level
        watermark_isr++;
        DAC_SR_REG(DAC1_BASE_PTR)= Clear_DACBFWMF ;
    }

//DAC_SR_REG(DAC1_BASE_PTR)=0x00;//clear all flags

    return ;
}

/**************************************************************/

void dac0_clk_enable (void ) {
    SIM_SCGC2 |= SIM_SCGC2_DAC0_MASK; //Allow clock to enable DAC0
}

void dac1_clk_enable(void) {
    SIM_SCGC2 |= SIM_SCGC2_DAC1_MASK; //Allow clock to enable DAC1
}

void dac0_1_clk_enable(void) {
    dac0_clk_enable();
    dac1_clk_enable();
}

void dacx_init(void) {
    DAC0_C0 |= DAC_C0_DACEN_MASK ;
}

void VREF_Init(void) {
    SIM_SCGC4 |= SIM_SCGC4_VREF_MASK ;
    VREF_SC = 0x81 ;// Enable Vrefo and select internal mode
    //VREF_SC = 0x82; // Tight-regulation mode buffer enabled is reconmended over low buffered mode
    while (!(VREF_SC & VREF_SC_VREFST_MASK)  ) {} // wait till the VREFSC is stable
}

void DAC12_buffered (DAC_MemMapPtr dacx_base_ptr, byte WatermarkMode, byte BuffMode, byte Vreference, byte TrigMode, byte BuffInitPos,byte BuffUpLimit) {

    DAC_C0_REG(dacx_base_ptr) = (
                                    DAC_BFB_PTR_INT_DISABLE |
                                    DAC_BFT_PTR_INT_DISABLE |
                                    DAC_BFWM_INT_DISABLE  |
                                    DAC_HP_MODE    |
                                    DAC_SW_TRIG_STOP |
                                    TrigMode |
                                    Vreference |
                                    DAC_ENABLE    // DAC enalbed
                                );

    if ( Vreference == DAC_SEL_VREFO ) {
        VREF_Init();
    }// end of if


    DAC_C1_REG(dacx_base_ptr) = (
                                    DAC_BF_ENABLE  |  //Buffer Enabled
                                    WatermarkMode |  // set 1, 2, 3,or 4 word(s) mode
                                    BuffMode        // set traversal mode, normal, swing, or onetime
                                ) ;

    DAC_C2_REG(dacx_base_ptr) = BuffInitPos | BuffUpLimit;

}//end of DAC12_buffered

void DAC12_Buff_Init_Plus256(DAC_MemMapPtr dacx_base_ptr) {
    int data = 0;
    byte i =  0 ;
    //Uncomment the follows to test for buffer mode
    data = 0 ;
    // for loop: Initializes the buffer words so that next buffer has an increment of 256 except last one (255)
    for (i=0 ; i < 16 ; i++) {

        if(i == 15) { //Last buffer.The last word buffer (16th word) will have the value (16*0xff)-1 = 4096-1=4095=0xFFF.
            data += 255;
        }
        else { //Not last buffer.The next word buffer will have increment of 0xFF from previous word buffer.
            data += 256;
        }  //end of if-else statement

        SET_DACx_BUFFER( dacx_base_ptr, i, data);

    }// end of for loop
}//end of DAC12_Buff_Init_Plus256

void DAC12_Buff_Init_Plus256Sqr(DAC_MemMapPtr dacx_base_ptr) {
    byte i = 0;
    int data = 4095;
    for (i=0; i < 8; i++) {
        SET_DACx_BUFFER(dacx_base_ptr, i, data);
    }
    data = 0;
    for (i=8; i < 16; i++) {
        SET_DACx_BUFFER(dacx_base_ptr, i, data);
    }
}

void DAC12_Buff_Init_PlusN(DAC_MemMapPtr dacx_base_ptr, uint32_t n) {
    if (n > 255) {
        n = 255;
    }
    uint32_t data = 0;
    byte i = 0;
    data = 0 ;
    // for loop: Initializes the buffer words so that next buffer has an increment of n
    for (i=0 ; i < 16 ; i++) {

        if(i == 15) { //Last buffer.The last word buffer (16th word) will have the value (16*0xff)-1 = 4096-1=4095=0xFFF.
            data += n;
        }
        else { //Not last buffer.The next word buffer will have increment of 0xFF from previous word buffer.
            data += n;
        }  //end of if-else statement

        SET_DACx_BUFFER(dacx_base_ptr, i, data);
    }
}//end of DAC12_Buff_Init_PlusN

void DAC12_Buff_Init_PlusNSqr(DAC_MemMapPtr dacx_base_ptr, uint32_t n) {
    if (n > 255) {
        n = 255;
    }
    byte i = 0;
    int data = 16*n;
    for (i=0; i < 8; i++) {
        SET_DACx_BUFFER(dacx_base_ptr, i, data);
    }
    data = 0;
    for (i=8; i < 16; i++) {
        SET_DACx_BUFFER(dacx_base_ptr, i, data);
    }
}

int SET_DACx_BUFFER( DAC_MemMapPtr dacx_base_ptr, byte dacindex, int buffval) {
    int temp = 0 ;
    // initialize all 16 words buffer with a the value buffval
    DAC_DATL_REG(dacx_base_ptr, dacindex)  =   (buffval&0x0ff);
    DAC_DATH_REG(dacx_base_ptr, dacindex)  =   (buffval&0xf00) >>8;
    temp =( DAC_DATL_REG(dacx_base_ptr, dacindex)|( DAC_DATH_REG(dacx_base_ptr, dacindex)<<8));
    //if(temp != buffval){ while(1){} }
    return temp ;
}// end of SET_DACx_BUFFER


void DAC12_HWTrigBuff(DAC_MemMapPtr dacx_base_ptr, byte BuffMode, byte Vreference, byte TrigMode, byte BuffInitPos, byte BuffUpLimit) {
    /*CASE: user is changing waveType*/
    if (g_sw == 'w') {
        if (curWaveIndex == NUMWAVES) {
            curWaveIndex = 0;
        }
        else {
            curWaveIndex ++;
        }
        return;
    }
    //Attack
    DAC12_buffered(dacx_base_ptr,0, BuffMode, Vreference, TrigMode,BuffInitPos, BuffUpLimit);
    if (curWaveIndex == SQUARE) {
        DAC12_Buff_Init_Plus256Sqr(dacx_base_ptr);//init buffer to with 256 increment with following values word 0(=256), Word 1 (=256+256) .... to word 15 (=4096)
    }
    else {
        DAC12_Buff_Init_Plus256(dacx_base_ptr);//init buffer to with 256 increment with following values word 0(=256), Word 1 (=256+256) .... to word 15 (=4096)
    }
    //Initialize PDB for DAC hardware trigger
    PDB_DAC0_TriggerInit();
    PDB_DAC1_TriggerInit();
    if (g_relTim == 0) {
        return;
    }
    else {
        //Sustain
        uint32_t ct = g_relTim << 10;
        //busy wait loop before release envelope
        for (int i = 0; i < ct; i++) {
            ;
        }
        //Release
        uint16_t n = 256;
        while (n > 0) {
            if (curWaveIndex == SQUARE) {
                DAC12_Buff_Init_PlusNSqr(dacx_base_ptr, n);//init buffer to n ... 16n increments, square wave
            }
            else {
                DAC12_Buff_Init_PlusN(dacx_base_ptr, n);//init buffer to n ... 16n increments, saw wave
            }
            //Initialize PDB for DAC hardware trigger
            PDB_DAC0_TriggerInit();
            PDB_DAC1_TriggerInit();
            //busy wait loop before decrementing voltage of DAC
            for (int j = 0; j < ct; j++) {
                ;
            }
            //n scales down by factor of 2
            n = n >> 1;
        }
        g_sw ='0';
    }
} //end of DAC12_HWTrigBuff

void PDB_DAC0_TriggerInit(void) {
    SIM_SCGC6 |= SIM_SCGC6_PDB_MASK; // enable system clock to PDB
    PDB0_SC = PDB_SC_PDBEN_MASK; // enable PDB module
    if (g_sw == 'a') {
        PDB0_DACINT0 = NOTEA4;
    }
    else if (g_sw == 'g') {
        PDB0_DACINT0 = NOTEG4;
    }
    else if (g_sw == 'f') {
        PDB0_DACINT0 = NOTEF4;
    }
    else if (g_sw == 'e') {
        PDB0_DACINT0 = NOTEE4;
    }
    else if (g_sw == 'd') {
        PDB0_DACINT0 = NOTED4;
    }
    else if (g_sw == 'c') {
        PDB0_DACINT0 = NOTEC4;
    }
    else if (g_sw == 'b') {
        PDB0_DACINT0 = NOTEB3;
    }
    else {
        PDB0_DACINT0 = 600; //TODO: errcheck
    }
    PDB0_SC |=PDB_SC_LDOK_MASK;// load the value assigned to PDB_DACINT0 to register
    PDB0_SC |= PDB_SC_TRGSEL(15); // software trigger
    PDB0_SC |=PDB_SC_CONT_MASK;// CONT mode
    PDB0_DACINTC0 |= PDB_INTC_TOE_MASK;// DAC output delay from PDB Software trigger
    PDB0_SC |= PDB_SC_SWTRIG_MASK;// reset counter to 0. This triggers the PDB input.
} //end of PDB_DAC_TriggerInit

void PDB_DAC1_TriggerInit(void) {
    SIM_SCGC6 |= SIM_SCGC6_PDB_MASK; // enable system clock to PDB
    PDB0_SC = PDB_SC_PDBEN_MASK; // enable PDB module
    PDB0_DACINT1 = 4545;// Effective after writting PDBSC_DACTOE = 1, DAC output changes are base on the interval defined by this value
    PDB0_SC |=PDB_SC_LDOK_MASK;// load the value assigned to PDB_DACINT1  to register
    PDB0_SC |= PDB_SC_TRGSEL(15); // software trigger
    PDB0_SC |=PDB_SC_CONT_MASK;// CONT mode
    PDB0_DACINTC1 |= PDB_INTC_TOE_MASK;// DAC output delay from PDB Software trigger
    PDB0_SC |= PDB_SC_SWTRIG_MASK;// reset counter to 0. This triggers the PDB input.
} //end of PDB_DAC_TriggerInit

void DACx_reset_dac0_1_reg_values(void) {
    DACx_register_reset_por_values (DAC0_BASE_PTR);
    DACx_register_reset_por_values (DAC1_BASE_PTR);
}//end of DACx_reset_dac0_1_reg_values


void DACx_register_reset_por_values (DAC_MemMapPtr dacx_base_ptr) {
    unsigned char dacbuff_index = 0 ;

    for (dacbuff_index=0; dacbuff_index<16; dacbuff_index++) {
        SET_DACx_BUFFER( dacx_base_ptr, dacbuff_index, DACx_DAT_RESET);
    }

    DAC_SR_REG(dacx_base_ptr) = DACx_SR_RESET ;
    DAC_C0_REG(dacx_base_ptr) = DACx_C0_RESET ;
    DAC_C1_REG(dacx_base_ptr) = DACx_C1_RESET;
    DAC_C2_REG(dacx_base_ptr) = DACx_C2_RESET;
} //end of DACx_register_reset_por_values



void DAC12_Interrupt_Init(DAC_MemMapPtr dacx_base_ptr,byte watermark, byte zerobuffer, byte uplimitbuffer ) {
    if (dacx_base_ptr == DAC0_BASE_PTR ) {
        NVICICPR2 |= (1<<17);
        NVICISER2 |= (1<<17);
    } else {
        NVICICPR2 |= (1<<18);
        NVICISER2 |= (1<<18);
    }

    DAC_C0_REG(dacx_base_ptr) |= watermark |zerobuffer | uplimitbuffer ;
}

void DAC12_WatermarkBuffInterrupt(DAC_MemMapPtr dacx_base_ptr, byte WatermarkBuffMode,byte BuffMode, byte Vreference, byte TrigMode, byte BuffInitPos, byte BuffUpLimit) {
    DAC12_buffered(dacx_base_ptr,WatermarkBuffMode, BuffMode, Vreference, TrigMode,BuffInitPos, BuffUpLimit) ;
    DAC_SR_REG(dacx_base_ptr) = 0; //clear all the flags
    DAC12_Interrupt_Init(dacx_base_ptr, DAC_BFWM_INT_ENABLE, DAC_BFT_PTR_INT_ENABLE,DAC_BFB_PTR_INT_ENABLE);
    enable_interrupts();
    return;
}