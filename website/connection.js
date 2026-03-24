const SUPABASE_URL = 'YOUR_URL'
const SUPABASE_KEY = 'YOUR_ANON_KEY'
const supabaseDB = supabase.createClient(SUPABASE_URL, SUPABASE_KEY);

const GEMINI_API_KEY = "YOUR_API_KEY";
const genAI = new GoogleGenerativeAI(GEMINI_API_KEY);