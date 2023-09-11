// === RAYTO ===
use std::env;

#[derive(Copy, Clone)]
struct Point {
    x: f64,
    y: f64,
    z: f64
}

// n = number of segments
// r0 = starting radius
// r1 = ending radius
// theta0 = starting angle
// theta1 = ending angle
// x = x-position of point
// y = y-position of point
// h = height or thickness
// tex_top = top texture
// tex_bot = bottom texture
// tex_face = face texture
// tex_hidden = hidden textures (normally set to mtrl/invisible)

fn main() {
    let args: Vec<String> = env::args().collect();
    if args.len() < 13 {
        eprintln!("Not enough arguments.");
    }
    else {
        // get input parameters
        let n: i32 = args[1].parse().unwrap();
        let r0: f64 = args[2].parse().unwrap();
        let r1: f64 = args[3].parse().unwrap();
        let theta0: f64 = args[4].parse().unwrap();
        let theta1: f64 = args[5].parse().unwrap();
        let x: f64 = args[6].parse().unwrap();
        let y: f64 = args[7].parse().unwrap();
        let h: f64 = args[8].parse().unwrap();
        let tex_top: String = args[9].clone();
        let tex_bot: String = args[10].clone();
        let tex_face: String = args[11].clone();
        let tex_hidden: String = args[12].clone();

        // get delta values
        let dr = (r1-r0)/(n as f64);
        let dtheta = (theta1 - theta0)/(n as f64);

        // start map
        println!("{{");
        println!("\"classname\" \"worldspawn\"");
        println!("// This curve was generated using rayto");
        // create lumps
        for i in 0..n {
            let rstart = r0 + dr*(i as f64);
            let rend = r0 + dr*(i as f64 + 1.0);
            let thetastart = theta0 + dtheta*(i as f64);
            let thetaend = theta0 + dtheta*(i as f64 + 1.0);

            lump_tprism(rstart, rend, thetastart, thetaend, x, y, h, &tex_top, &tex_bot, &tex_face, &tex_hidden);
        }
        // end map
        print!("}}\n");
    }
}

fn lump_tprism(rstart: f64, rend: f64, thetastart: f64, thetaend: f64, x: f64, y: f64, h: f64, tex_top: &str, tex_bot: &str, tex_face: &str, tex_hidden: &str) {
    // start lump
    println!("{{");

    let x0 = rstart*f64::cos(thetastart*std::f64::consts::PI/180.0);
    let y0 = rstart*f64::sin(thetastart*std::f64::consts::PI/180.0);
    let x1 = rend*f64::cos(thetaend*std::f64::consts::PI/180.0);
    let y1 = rend*f64::sin(thetaend*std::f64::consts::PI/180.0);

    let pa = Point{x: x, y: y, z: 0.0};
    let pb = Point{x: x0, y: y0, z: 0.0};
    let pc = Point{x: x1, y: y1, z: 0.0};
    let pd = Point{x: x, y: y, z: h};
    let pe = Point{x: x0, y: y0, z: h};
    let pf = Point{x: x1, y: y1, z: h};

    // create all sides
    side((pa, pb, pe), &tex_hidden);
    side((pd, pf, pc), &tex_hidden);
    side((pc, pe, pb), &tex_face);
    side((pa, pc, pb), &tex_bot);
    side((pd, pe, pf), &tex_top);

    // end lump
    println!("}}");

}

fn side(points: (Point, Point, Point), texture: &str) {
    println!("( {:.6} {:.6} {:.6} ) ( {:.6} {:.6} {:.6} ) ( {:.6} {:.6} {:.6} ) {} 0 0 0 0.500000 0.500000 0 0 0",
    points.0.x, points.0.y, points.0.z,
    points.1.x, points.1.y, points.1.z,
    points.2.x, points.2.y, points.2.z,
    texture);
}
