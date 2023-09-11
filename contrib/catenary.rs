// === CATENARY ===

// Math resources used:
// https://en.wikipedia.org/wiki/Catenary
// https://math.stackexchange.com/questions/1000447/finding-the-catenary-curve-with-given-arclength-through-two-given-points
// https://foggyhazel.wordpress.com/2018/02/12/catenary-passing-through-2-points/ 
use std::env;

#[derive(Copy, Clone)]
struct Point {
    x: f64,
    y: f64,
    z: f64
}

// n = number of segments
// x0 = starting horizontal position of curve
// z0 = starting height of curve
// x1 = ending horizontal position of curve
// z1 = ending height of curve
// s = length of the curve (i.e. how long your rope is)
// width = how wide the resulting curve will be (along y-axis)
// thickness = how thick the resulting curve will be
// tex_top = top texture, i.e. mtrl/wood
// tex_bot = bottom texture
// tex_right = right texture
// tex_left = left texture
// tex_hidden = texture for hidden faces, normally set to mtrl/invisible
// initial_guess = the starting value for Newton's method. Try 1.0 and change it if it doesn't converge.

fn main() {
    let args: Vec<String> = env::args().collect();
    if args.len() < 15 {
        eprintln!("Not enough arguments.");
    }
    else {
        // get input arguments
        let n: i32 = args[1].parse().unwrap();
        let x0: f64 = args[2].parse().unwrap();
        let z0: f64 = args[3].parse().unwrap();
        let x1: f64 = args[4].parse().unwrap();
        let z1: f64 = args[5].parse().unwrap();
        let s: f64 = args[6].parse().unwrap();
        let width: f64 = args[7].parse().unwrap();
        let thickness: f64 = args[8].parse().unwrap();
        let tex_top: String = args[9].clone();
        let tex_bot: String = args[10].clone();
        let tex_right: String = args[11].clone();
        let tex_left: String = args[12].clone();
        let tex_hidden: String = args[13].clone();
        let initial_guess: f64 = args[14].parse().unwrap();
        let v = z1-z0;
        let h = x1-x0;

        if s < f64::sqrt(f64::powi(z1-z0, 2) + f64::powi(x1-x0, 2)) {
            eprintln!("Given length is too short, must be at least {}", f64::sqrt(f64::powi(z1-z0, 2) + f64::powi(x1-x0, 2)));
        }
        else {

            // where the magic happens in finding the catenary parameters
            if let Ok(a) = newton_a(v, h, s, x0, z0, x1, z1, initial_guess) {

                // Find the other catenary parameters. Thankfully these aren't as bad.
                let k: f64 = x0 + 0.5 * (h - a*f64::ln((s+v)/(s-v)));
                let c: f64 = z0 - a * f64::cosh((x0-k)/a);
                
                // Split up into discrete segments.
                let dx = (x1-x0)/(n as f64);

                // start map
                println!("{{");
                println!("\"classname\" \"worldspawn\"");
                println!("// This curve was generated using catenary");

                // create lumps
                for i in 0..n {
                    
                    // start lump
                    println!("{{");

                    // xs = x start, xe = x end
                    // zs0/ze0 = z bottom, zs1/ze1 = z top
                    let xs = x0 + dx*(i as f64);
                    let xe = x0 + dx*(i as f64 + 1.0);
                    let zs0 = catenary(xs, a, k, c) - thickness;
                    let zs1 = catenary(xs, a, k, c);
                    let ze0 = catenary(xe, a, k, c) - thickness;
                    let ze1 = catenary(xe, a, k, c);

                    let pa = Point{x: xe, y: 0.0, z: ze0};
                    let pb = Point{x: xs, y: 0.0, z: zs0};
                    let pc = Point{x: xs, y: width, z: zs0};
                    let pd = Point{x: xe, y: width, z: ze0};
                    let pe = Point{x: xe, y: 0.0, z: ze1};
                    let pf = Point{x: xs, y: 0.0, z: zs1};
                    let pg = Point{x: xs, y: width, z: zs1};
                    let ph = Point{x: xe, y: width, z: ze1};

                    side((pa, pb, pf), &tex_right);
                    side((pc, pd, ph), &tex_left);
                    side((pb, pc, pg), &tex_hidden);
                    side((pe, ph, pd), &tex_hidden);
                    side((pa, pd, pc), &tex_bot);
                    side((pe, pf, pg), &tex_top);

                    // end lump
                    println!("}}");
                }

                // end map
                println!("}}");
            }
            else {
                eprintln!("Newton's method failed to find a sufficiently accurate solution.");
            }
        }
    }
}

fn catenary(x: f64, a: f64, k: f64, c: f64) -> f64 {
    a*f64::cosh((x-k)/a)+c
}

// ---------- FINDING THE CATENARY PARAMETER "a" ----------

// Newton's method. Has the potential to fail.
fn newton_a(v: f64, h: f64, s: f64, x0: f64, z0: f64, x1: f64, z1: f64, initial_guess: f64) -> Result<f64, ()> {

    let iteration_limit = 1000;
    // Limit for how inaccurate our points can be. We print to six decimal places,
    // so this should be OK.. 
    let epsilon: f64 = f64::powi(10.0, -9);

    // Initial guess
    let mut b_i: f64 = initial_guess / h;
    let mut b_ip1: f64 = b_i;
    let mut icount: i32 = 0;
    while catenary_bounds_err(x0, z0, x1, z1, b_ip1*h, s) > epsilon && icount < iteration_limit {
        eprintln!("Iteration {}: maximum error is {}", icount, catenary_bounds_err(x0, z0, x1, z1, b_ip1*h, s));
        b_i = b_ip1;
        b_ip1 = b_i - f2b(b_i, v, h, s)/df2b(b_i);
        icount += 1;
    }
    eprintln!("Iteration {}: maximum error is {}", icount, catenary_bounds_err(x0, z0, x1, z1, b_ip1*h, s));
    //a = bh
    if icount >= iteration_limit || !f64::is_finite(catenary_bounds_err(x0, z0, x1, z1, b_ip1*h, s)) {
        Err(())
    }
    else {
        Ok(b_ip1*h)  
    }
}

fn f2b(b: f64, v: f64, h: f64, s: f64) -> f64 {
    1.0/f64::sqrt(2.0*b*f64::sinh(1.0/(2.0*b))-1.0) - 1.0/f64::sqrt(f64::sqrt(f64::powi(s, 2) - f64::powi(v, 2))/h-1.0)
}

fn df2b(b: f64) -> f64 {
    let m = 1.0/(2.0*b);
    (m*f64::cosh(m)-f64::sinh(m))*f64::powf(1.0/m*f64::sinh(m)-1.0, -1.5)
}

fn catenary_bounds_err(x0: f64, z0: f64, x1: f64, z1: f64, a: f64, s: f64) -> f64 {
    let v = z1-z0;
    let h = x1-x0;
    let k: f64 = x0 + 0.5 * (h - a*f64::ln((s+v)/(s-v)));
    let c: f64 = z0 - a * f64::cosh((x0-k)/a);
    let error0 = f64::abs(catenary(x0, a, k, c) - z0);
    let error1 = f64::abs(catenary(x1, a, k, c) - z1);
    f64::max(error0, error1)
}

fn side(points: (Point, Point, Point), texture: &str) {
    println!("( {:.6} {:.6} {:.6} ) ( {:.6} {:.6} {:.6} ) ( {:.6} {:.6} {:.6} ) {} 0 0 0 0.500000 0.500000 0 0 0",
    points.0.x, points.0.y, points.0.z,
    points.1.x, points.1.y, points.1.z,
    points.2.x, points.2.y, points.2.z,
    texture);
}
