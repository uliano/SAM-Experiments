import pypdf, sys
sys.stdout.reconfigure(encoding='utf-8')
reader = pypdf.PdfReader(r'c:\Users\uliano\Documents\SAM-Experiments\docs\SAMC20_C21__Data_Sheet.pdf')
for i in range(704, 740):
    text = reader.pages[i].extract_text() or ''
    if any(k in text for k in ['NPWM','MPWM','Waveform','WO[','waveform','wavegen','WAVEGEN']):
        print(f'--- page {i+1} ---')
        print(text[:4000].encode('utf-8','replace').decode('utf-8'))
        print()
