import pypdf, sys
sys.stdout.reconfigure(encoding='utf-8')
reader = pypdf.PdfReader(r'c:\Users\uliano\Documents\SAM-Experiments\docs\SAMC20_C21__Data_Sheet.pdf')
# Read CCL INSEL table pages
for i in range(853, 875):
    text = reader.pages[i].extract_text() or ''
    if 'INSEL' in text or 'TC' in text or 'TCC' in text or 'TRUTH' in text:
        print(f'--- page {i+1} ---')
        print(text[:5000].encode('utf-8', errors='replace').decode('utf-8'))
        print()
