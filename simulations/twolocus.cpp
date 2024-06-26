#include <iostream>
#include <cmath>
#include <vector>

#include "stats.hpp" // to help with random number generation

inline double single_locus_sfs(double x, double Ns){
    
    if(Ns>1e-03){
        // positive selection
        double exponent = 2*Ns*(1-x);
        if(exponent<1e-03){
            return 2*Ns/x;
        }
        else{
            return (1-exp(-exponent))/x/(1-x);
        }
    }
    else if(Ns<-1e-03){
        // negative selection
        double absNs = Ns*(-1);
        double exponent = 2*absNs*(1-x);
        if(exponent<1e-03){
            return 2*absNs*exp(-2*absNs)/x;
        }
        else{
            return (exp(-2*absNs*x)-exp(-2*absNs))/x/(1-x);
        }
    }
    else{
        // neutral
        return 1.0/x;
    }
}

int main(int argc, char * argv[]){

    // Make sure there are enough command line arguments
    if(argc < 8){
        std::cout << "usage: " << argv[0] << " num_runs dt N s1 s2 eps r [mu]" << std::endl;
        return 1;
    }

    // Read in parameters 
    const long long num_runs = (long long) atof(argv[1]); // number of replicate populations to evolve
    const int dt = (int) atof(argv[2]);
    const int N = (int) atof(argv[3]);
    const double s1 = -1*atof(argv[4]); // record negative value
    const double s2 = -1*atof(argv[5]); // negative
    const double eps = -1*atof(argv[6]); // negative
    const double r = atof(argv[7]);

    double mu = 0; // for compatibility with other parameter combinations
    if (argc == 9){
        // specify the mutation rate
        mu = atof(argv[8]);
    }
    
    //std::cerr << num_runs << std::endl;
    
    const double reporting_probability = 1.0/dt; 
    
    // Calculate Wrightian fitnesses (not sure this is really necessary, but old habit)
    const double W10 = std::exp(s1);
    const double W01 = std::exp(s2);
    const double W11 = std::exp(s1+s2+eps);
    const double W00 = 1.0;
    
    // Create and seed random number generator
    Random random = create_random();
 
    // SFS for first locus
    std::vector<double> pn1s;
    // SFS for second locus
    std::vector<double> pn2s;
    
    double total_pn1=0;
    double total_pn2=0; 
    
    // Manually enter first entry
    // since it is *polymorphic* SFS, can't have zero!
    pn1s.push_back(0); 
    pn2s.push_back(0);
    // Now do rest
    for(int n=1;n<N;++n){
    
        double pn1 = single_locus_sfs(n*1.0/N, N*s1);
        total_pn1 += pn1;
        pn1s.push_back( pn1 );
        
        double pn2 = single_locus_sfs(n*1.0/N, N*s2);
        total_pn2 += pn2;
        pn2s.push_back( pn2 );
        
    }
 
    // Now construct sampling distributions
    
    // probability that locus1 was mutated before locus2
    std::bernoulli_distribution first_site_mutated_first(total_pn1/(total_pn1+total_pn2));
    
    // sampling distributions for SFS
    std::discrete_distribution<int> draw_sfs1 (pn1s.begin(), pn1s.end());
    std::discrete_distribution<int> draw_sfs2 (pn2s.begin(), pn2s.end());
    std::cout << "// " << N*s1 << " " << N*s2 << " " << N*eps << " " << N*r << std::endl;
    for(long long run_idx=0;run_idx<num_runs;++run_idx){
        // Evolve a replicate population
        
        // haplotype counts in population
        // (should sum to N)
        int n00, n10, n01, n11;

        // initialize population
        if(first_site_mutated_first(random)){
            // first mutation was at site 1
            // draw from SFS
            
            n10 = draw_sfs1(random);
            
            if(sample_uniform(random)<(n10*1.0/N)){
                // n11 individual produced
                n11 = 1;
                n01 = 0;
                n10 -= 1;
            }
            else{
                // n01 individual produced
                // from wildtype background
                n11 = 0;
                n01 = 1;
            }
        }
        else{
            // first mutation was at site 2
            // draw from SFS
            
            n01 = draw_sfs2(random);
            
            if(sample_uniform(random)<(n01*1.0/N)){
                // n11 individual produced
                // from a n01 individual
                n11 = 1;
                n10 = 0;
                n01 -= 1;
            }
            else{
                // n01 individual produced
                // from wildtype background
                n11 = 0;
                n10 = 1;
            }
        }
        // fill remaining haplotype by normalization    
        n00 = N-n11-n10-n01;
        
        // print a header for each one
        //std::cout << "// " << N*s1 << " " << N*s2 << " " << N*eps << " " << N*r << std::endl;
        
        // as long as there are polymorphisms at both sites
        
        while( ((n11+n10)>0) && ((n11+n01)>0) && ((n10+n00)>0) && ((n01+n00)>0) ) {
        
            // total population size
            double ntot = n11+n10+n01+n00;
            
            double f11 = n11/ntot;
            double f10 = n10/ntot;
            double f01 = n01/ntot;
            double f00 = 1-f11-f10-f01;
            
            double Wavg = W11*f11+W10*f10+W01*f01+W00*f00;
            double D = f11*f00 - f10*f01;
            
            if(sample_uniform(random) < reporting_probability){
                std::cout << f11 << " " << f10 << " " << f01 << std::endl;
            }
            
            // calculate expected values at next generation
            // (deterministic 2 locus dynamics)
            // also incorporating forward mutations
            double expected_f11 = (W11/Wavg)*f11 - r*D + mu*(f10+f01) - 2*mu*f11;
            double expected_f10 = (W10/Wavg)*f10 + r*D + mu*(f00+f11) - 2*mu*f10;
            double expected_f01 = (W01/Wavg)*f01 + r*D + mu*(f00+f11) - 2*mu*f01;
            double expected_f00 = (W00/Wavg)*f00 - r*D + mu*(f01+f10) - 2*mu*f00;
            
            // sample next generation
            n11 = sample_poisson(random, N*expected_f11);
            n10 = sample_poisson(random, N*expected_f10);
            n01 = sample_poisson(random, N*expected_f01);
            n00 = sample_poisson(random, N*expected_f00);
            
        } // repeat until one site goes extinct

    } // repeat until simulation is terminated.

    return 0;
}

