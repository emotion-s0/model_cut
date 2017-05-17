//  the efficiency may be a bottleneck, as far as I know, the complete coding frame needs to be 30MS, so slow isn't it ?
//  We do not want to manage the problem of overtime, 
//  so basically can not read the data will have been dead lock procedures, please note
// ================================================================================




// ========================================================================
// Read PLC's relay M.
// ========================================================================
bool plc_rm (int address, bool *bit) {

  BOOL io_success_;
  DWORD rv_numbs = 0;
  BYTE varsbuf[128];
  struct read_section2 rcrs2;
  char obt;

//uint16_t rv_numbs2 = 0;
  uint16_t rv_numbs0 = 0;
  int e = fx1s_makersec (& rcrs2, FX1S_REGISTER_FIELD_M, 1, & rv_numbs0, address);
  assert (e == FX1S_OK);

  io_success_ = WriteFile (SCP_misc.commport, & rcrs2, sizeof (struct read_section), & rv_numbs, NULL);
  if (io_success_ == FALSE) return FALSE;
  if (sizeof (struct read_section) != rv_numbs) return FALSE;
  io_success_ = ReadFile  (SCP_misc.commport, & varsbuf [0], rv_numbs0, & rv_numbs, NULL);
  if (io_success_ == FALSE) return FALSE;
  if (rv_numbs != rv_numbs0) return FALSE;
  if (fx1s_makebuf (& varsbuf [0], & obt, sizeof (varsbuf), 1, 1) != FX1S_OK)
    return FALSE;
  else 
  {
    bit [0] = !! (obt & (1 << (address & 7)));
    return TRUE;
  }
}

// ========================================================================
// Read PLC's output Y. ( Remember!!! the Y index is octal )
// ========================================================================
bool plc_ry (int address, bool *bit) {

  BOOL io_success_;
  DWORD rv_numbs = 0;
  BYTE varsbuf[128];
  struct read_section2 rcrs2;
  char obt;

//uint16_t rv_numbs2 = 0;
  uint16_t rv_numbs0 = 0;
  int e = fx1s_makersec (& rcrs2, FX1S_REGISTER_FIELD_Y_OUT, 1, & rv_numbs0, address);
  assert (e == FX1S_OK);

  io_success_ = WriteFile (SCP_misc.commport, & rcrs2, sizeof (struct read_section), & rv_numbs, NULL);
  if (io_success_ == FALSE) return FALSE;
  if (sizeof (struct read_section) != rv_numbs) return FALSE;
  io_success_ = ReadFile  (SCP_misc.commport, & varsbuf [0], rv_numbs0, & rv_numbs, NULL);
  if (io_success_ == FALSE) return FALSE;
  if (rv_numbs != rv_numbs0) return FALSE;
  if (fx1s_makebuf (& varsbuf [0], & obt, sizeof (varsbuf), 1, 1) != FX1S_OK)
    return FALSE;
  else 
  {
    bit [0] = !! (obt & (1 << ( vailed8 (address) & 7)));
    return TRUE;
  }
}

// ========================================================================
// Read PLC's data register
// ========================================================================
bool plc_rdw (int address, short *data) {

  BOOL io_success_;
  DWORD rv_numbs = 0;
  BYTE varsbuf[128];
  struct read_section2 rcrs2;

//uint16_t rv_numbs2 = 0;
  uint16_t rv_numbs0 = 0;
  int e = fx1s_makersec (& rcrs2, FX1S_REGISTER_FIELD_D, 1, & rv_numbs0, address);
  assert (e == FX1S_OK);

  io_success_ = WriteFile (SCP_misc.commport, & rcrs2, sizeof (struct read_section), & rv_numbs, NULL);
  if (io_success_ == FALSE) return FALSE;
  if (sizeof (struct read_section) != rv_numbs) return FALSE;
  io_success_ = ReadFile  (SCP_misc.commport, & varsbuf [0], rv_numbs0, & rv_numbs, NULL);
  if (io_success_ == FALSE) return FALSE;
  if (rv_numbs != rv_numbs0) return FALSE;
  if (fx1s_makebuf (& varsbuf [0], data, sizeof (varsbuf), 2, 2) != FX1S_OK)
    return FALSE;
  else 
    return TRUE;
}

// ========================================================================
// Read PLC's data register level2
// ========================================================================
bool plc_rdd (int address, int *data) {

  BOOL io_success_;
  DWORD rv_numbs = 0;
  BYTE varsbuf[128];
  struct read_section2 rcrs2;

//uint16_t rv_numbs2 = 0;
  uint16_t rv_numbs0 = 0;
  int e = fx1s_makersec (& rcrs2, FX1S_REGISTER_FIELD_D, 2, & rv_numbs0, address);
  assert (e == FX1S_OK);

  io_success_ = WriteFile (SCP_misc.commport, & rcrs2, sizeof (struct read_section), & rv_numbs, NULL);
  if (io_success_ == FALSE) return FALSE;
  if (sizeof (struct read_section) != rv_numbs) return FALSE;
  io_success_ = ReadFile  (SCP_misc.commport, & varsbuf [0], rv_numbs0, & rv_numbs, NULL);
  if (io_success_ == FALSE) return FALSE;
  if (rv_numbs != rv_numbs0) return FALSE;
  if (fx1s_makebuf (& varsbuf [0], data, sizeof (varsbuf), 4, 4) != FX1S_OK)
    return FALSE;
  else 
    return TRUE;
}

// ========================================================================
// Read PLC's high speed pulse.
// ========================================================================
bool plc_rc32d (int address, int *data) {

  BOOL io_success_;
  DWORD rv_numbs = 0;
  BYTE varsbuf[128];
  struct read_section2 rcrs2;

//uint16_t rv_numbs2 = 0;
  uint16_t rv_numbs0 = 0;
  int e = fx1s_makersec (& rcrs2, FX1S_REGISTER_FIELD_C32, 1, & rv_numbs0, address);
  assert (e == FX1S_OK);

  io_success_ = WriteFile (SCP_misc.commport, & rcrs2, sizeof (struct read_section), & rv_numbs, NULL);
  if (io_success_ == FALSE) return FALSE;
  if (sizeof (struct read_section) != rv_numbs) return FALSE;
  io_success_ = ReadFile  (SCP_misc.commport, & varsbuf [0], rv_numbs0, & rv_numbs, NULL);
  if (io_success_ == FALSE) return FALSE;
  if (rv_numbs != rv_numbs0) return FALSE;
  if (fx1s_makebuf (& varsbuf [0], data, sizeof (varsbuf), 4, 4) != FX1S_OK)
    return FALSE;
  else 
    return TRUE;
}

// ========================================================================
// Write PLC's relay M
// ========================================================================
bool plc_wm (int address, bool bit) {

  BOOL io_success_;
  DWORD rv_numbs = 0;
  BYTE varsbuf[128];
  struct read_section2 rcrs2;
  char obt;

//uint16_t rv_numbs2 = 0;
  uint16_t rv_numbs0 = 0;
  int e = fx1s_makersec (& rcrs2, FX1S_REGISTER_FIELD_M, 1, & rv_numbs0, address);
  assert (e == FX1S_OK);

  io_success_ = WriteFile (SCP_misc.commport, & rcrs2, sizeof (struct read_section), & rv_numbs, NULL);
  if (io_success_ == FALSE) return FALSE; 
  io_success_ = ReadFile  (SCP_misc.commport, & varsbuf [0], rv_numbs0, & rv_numbs, NULL);
  if (io_success_ == FALSE) return FALSE;
  if (rv_numbs != rv_numbs0) return FALSE;
  if (fx1s_makebuf (& varsbuf [0], & obt, sizeof (varsbuf), 1, 1) != FX1S_OK)
    return FALSE;
  else 
  {
    obt &= ~ (1 << (address & 7));
    obt |=     ( ( !!bit) << (address & 7));

    // write buf. 
    e = fx1s_makewsec (& varsbuf[0], & obt, FX1S_REGISTER_FIELD_M, 1, & rv_numbs0, address);
    assert (e == FX1S_OK);
    io_success_ = WriteFile (SCP_misc.commport, & varsbuf[0], rv_numbs0, & rv_numbs, NULL);
    if (io_success_ == FALSE) return FALSE;
    if (rv_numbs != rv_numbs0) return FALSE;
    io_success_ = ReadFile  (SCP_misc.commport, & varsbuf [0], 1, & rv_numbs, NULL);
    if (io_success_ == FALSE) return FALSE;
    if (rv_numbs != 1 || varsbuf[0] != SECTION_LINK_ACK) return FALSE;
    return TRUE;
  }
}

// ========================================================================
// Write PLC's output Y
// ========================================================================
bool plc_wy (int address, bool bit) {

  BOOL io_success_;
  DWORD rv_numbs = 0;
  BYTE varsbuf[128];
  struct read_section2 rcrs2;
  char obt;

//uint16_t rv_numbs2 = 0;
  uint16_t rv_numbs0 = 0;
  int e = fx1s_makersec (& rcrs2, FX1S_REGISTER_FIELD_Y_OUT, 1, & rv_numbs0, address);
  assert (e == FX1S_OK);

  io_success_ = WriteFile (SCP_misc.commport, & rcrs2, sizeof (struct read_section), & rv_numbs, NULL);
  if (io_success_ == FALSE) return FALSE;
  if (sizeof (struct read_section) != rv_numbs) return FALSE;
  io_success_ = ReadFile  (SCP_misc.commport, & varsbuf [0], rv_numbs0, & rv_numbs, NULL);
  if (io_success_ == FALSE) return FALSE;
  if (rv_numbs != rv_numbs0) return FALSE;
  if (fx1s_makebuf (& varsbuf [0], & obt, sizeof (varsbuf), 1, 1) != FX1S_OK)
    return FALSE;
  else 
  {
    obt &= ~ (1 << (vailed8 (address) & 7));
    obt |=     ( ( !!bit) << (vailed8 (address) & 7));

    // write buf. 
    e = fx1s_makewsec (& varsbuf[0], & obt, FX1S_REGISTER_FIELD_Y_OUT, 1, & rv_numbs0, address);
    assert (e == FX1S_OK);
    io_success_ = WriteFile (SCP_misc.commport, & varsbuf[0], rv_numbs0, & rv_numbs, NULL);
    if (io_success_ == FALSE) return FALSE;
    if (rv_numbs != rv_numbs0) return FALSE;
    io_success_ = ReadFile  (SCP_misc.commport, & varsbuf [0], 1, & rv_numbs, NULL);
    if (io_success_ == FALSE) return FALSE;
    if (rv_numbs != 1 || varsbuf[0] != SECTION_LINK_ACK) return FALSE;
    return TRUE;
  }
}

// ========================================================================
// Write PLC's data register
// ========================================================================
bool plc_wdw (int address, short data) {

  BOOL io_success_;
  DWORD rv_numbs = 0;
  BYTE varsbuf[128];

  uint16_t rv_numbs0 = 0;
  int e;
  {
    // write buf. 
    e = fx1s_makewsec (& varsbuf[0], & data, FX1S_REGISTER_FIELD_D, 1, & rv_numbs0, address);
    assert (e == FX1S_OK);
    io_success_ = WriteFile (SCP_misc.commport, & varsbuf[0], rv_numbs0, & rv_numbs, NULL);
    if (io_success_ == FALSE) return FALSE;
    if (rv_numbs != rv_numbs0) return FALSE;
    io_success_ = ReadFile  (SCP_misc.commport, & varsbuf [0], 1, & rv_numbs, NULL);
    if (io_success_ == FALSE) return FALSE;
    if (rv_numbs != 1 || varsbuf[0] != SECTION_LINK_ACK) return FALSE;
    return TRUE;
  }
}

// ========================================================================
// Write PLC's data register level2
// ========================================================================
bool plc_wdd (int address, int data) {

  BOOL io_success_;
  DWORD rv_numbs = 0;
  BYTE varsbuf[128];

  uint16_t rv_numbs0 = 0;
  int e;
  {
    // write buf. 
    e = fx1s_makewsec (& varsbuf[0], & data, FX1S_REGISTER_FIELD_D, 2, & rv_numbs0, address);
    assert (e == FX1S_OK);
    io_success_ = WriteFile (SCP_misc.commport, & varsbuf[0], rv_numbs0, & rv_numbs, NULL);
    if (io_success_ == FALSE) return FALSE;
    if (rv_numbs != rv_numbs0) return FALSE;
    io_success_ = ReadFile  (SCP_misc.commport, & varsbuf [0], 1, & rv_numbs, NULL);
    if (io_success_ == FALSE) return FALSE;
    if (rv_numbs != 1 || varsbuf[0] != SECTION_LINK_ACK) return FALSE;
    return TRUE;
  }
}

// ========================================================================
// Write PLC's high spedd pulse
// ========================================================================
bool plc_wc32d (int address, int data) {

  BOOL io_success_;
  DWORD rv_numbs = 0;
  BYTE varsbuf[128];

  uint16_t rv_numbs0 = 0;
  int e;
  {
    // write buf. 
    e = fx1s_makewsec (& varsbuf[0], & data, FX1S_REGISTER_FIELD_C32, 1, & rv_numbs0, address);
    assert (e == FX1S_OK);
    io_success_ = WriteFile (SCP_misc.commport, & varsbuf[0], rv_numbs0, & rv_numbs, NULL);
    if (io_success_ == FALSE) return FALSE;
    if (rv_numbs != rv_numbs0) return FALSE;
    io_success_ = ReadFile  (SCP_misc.commport, & varsbuf [0], 1, & rv_numbs, NULL);
    if (io_success_ == FALSE) return FALSE;
    if (rv_numbs != 1 || varsbuf[0] != SECTION_LINK_ACK) return FALSE;
    return TRUE;
  }
}

// ========================================================================
// PLC foroce close.
// ========================================================================
bool plc_force_close (void) {
   
	 // force close PLC [by M8037] 
   return plc_wm (8037, TRUE);
}

// ========================================================================
// PLC foroce open.
// ========================================================================
bool plc_force_open (void) {

   // force open PLC [by M8035/M8036/M8037]
   bool t = plc_wm (8035, TRUE);
   t = t && plc_wm (8036, TRUE);
   t = t && plc_wm (8037, FALSE);
   return t;
}